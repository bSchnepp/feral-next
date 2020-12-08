/*
Copyright (c) 2019, Brian Schnepp

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute, execute,
and transmit the Software, and to prepare derivative works of the Software,
and to permit third-parties to whom the Software is furnished to do so, all
subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer, must
be included in all copies of the Software, in whole or in part, and all
derivative works of the Software, unless such copies or derivative works are
solely in the form of machine-executable object code generated by a source
language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY
DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
 */


#include <feral/stdtypes.h>

#include <mm/mm.h>
#include <mm/heap.h>

#include <feral/kern/krnlfuncs.h>

/* Well, it works... */
static AllocatorState CurrentState;

void InternalInitializeNode(
	Node *Current, Node *Previous, Node *Next, UINT64 Index);

void *InternalMmWorstCaseMalloc(Arena *CurArena, UINT64 Size);


/*  FIXME: Add poison values to prevent bad heap accesses. */

/* Assume insertion is valid, and that caller handles Arena. */
void InternalInitializeNode(
	Node *Current, Node *Previous, Node *Next, UINT64 Index)
{

	/* Assume creation is valid. */
	Current->Used = FALSE;
	Current->Previous = Previous;
	if (Previous != NULLPTR)
	{
		Previous->Next = Current;
		Current->Area = (Previous->Area + 1);
	}

	Current->Next = Next;
	if (Next != NULLPTR)
	{
		Next->Previous = Current;
	}

	Current->NodeIndex = Index;
}

/* NumArenas should be equal to SMT threads... but doesn't have to be. */
AllocatorState *MmCreateAllocatorState(
	UINT64 NumArenas, VOID *HeapArea, UINT_PTR HeapSize)
{
	/* Zero out the entire heap first thing. */
	KiSetMemoryBytes(HeapArea, 0, HeapSize);
	AllocatorState State;
	UINTN HeapAmount = ((UINT64)(HeapSize) / NumArenas);

	/* Each arena has an equal area of the heap. */
	State.sType = MM_STRUCTURE_TYPE_HEAP_ALLOCATOR;
	State.pNext = NULLPTR;

	Arena *ArenaStart = (Arena *)KERN_PHYS_TO_VIRT(HeapArea);
	/* Space to reserve for nodes. */
	UINT64 NodeSize = (HeapAmount / (NumArenas * sizeof(Node)));

	for (UINT64 Counter = 0; Counter < NumArenas; ++Counter)
	{
		/* Write to that area an Arena... */
		Arena Current;
		Current.Size = (HeapAmount / NumArenas);
		Current.ThreadIndex = Counter; /* Counter is also thread num */

		/*
			The first node needs to point to an area outside where
			nodes will ever be written to.
		 */
		UINT_PTR NodeChunkSize = HeapSize / (sizeof(Node));
		UINT_PTR *ReinterpretHeap
			= (UINT_PTR *)(&(ArenaStart[Counter]) + NumArenas);

		Current.Root = (Node *)(ReinterpretHeap);
		InternalInitializeNode(Current.Root, NULLPTR, NULLPTR, 0);
		/* Root has no prev, so set area manually. */
		Current.Root->Area = (VOID *)(ReinterpretHeap + NodeSize + 1);
		/* Gets a place which should be empty. */
		Current.NextToAllocate = ((Node *)(Current.Root)) + 1;

		UINT64 NodeAmt = sizeof(Node);
		KiSetMemoryBytes(Current.NextToAllocate, 0, NodeAmt);
		Current.NextToAllocate->Previous = Current.Root;
		ArenaStart[Counter] = Current;
	}
	State.Arenas = ArenaStart;
	CurrentState = State;
	return &CurrentState;
}

void *InternalMmWorstCaseMalloc(Arena *CurArena, UINT64 Size)
{
	UINT64 RequiredNodes = (Size / ALLOC_BLOCK_SIZE) + 1;
	/* Go up one by one from root. */

	for (Node *Indexer = CurArena->Root; Indexer->Last != TRUE;
		Indexer = Indexer->Next)
	{
		if (Indexer->Used)
		{
			continue;
		}

		/* Check if there's the needed adjacent nodes... */
		UINT64 Count = 0;
		BOOL Okay = TRUE;
		for (Node *Subindexer = Indexer;
			(((Count++) < RequiredNodes)
				&& Subindexer->Last != TRUE);
			Subindexer = Subindexer->Next)
		{
			if (Subindexer->Used)
			{
				Okay = FALSE;
				break;
			}
		}

		if (Okay)
		{
			/* Set it up! */
			Node *Current = Indexer;
			Node *Previous = Indexer->Previous;
			Node *Next = Indexer->Next;
			UINT64 Index = Previous->NodeIndex + 1;
			InternalInitializeNode(Current, Previous, Next, Index);

			/* And reuse the subindexer code to mark as used. */

			for (Node *Subindexer = Current;
				(((Count++) < RequiredNodes)
					&& Subindexer->Last != TRUE);
				Subindexer = Subindexer->Next)
			{
				Subindexer->Used = TRUE;
			}
			return Current->Area;
		}
	}
	return (void *)(0);
}


/* FIXME: The thread number is ignored. */
void *MmKernelMalloc(UINT64 Size)
{
	/* FIXME: Area isn't mapped. */
	/* Request a chunk of some size. */
	UINT64 CurrentThread = 0;
	Arena *ThreadArena = &(CurrentState.Arenas[CurrentThread]);
	Node *Attempt = ThreadArena->NextToAllocate;
	if (Attempt && Attempt->Last)
	{
		return InternalMmWorstCaseMalloc(ThreadArena, Size);
	}

	/* Allocate it! */
	UINT64 BlockIncrement = (Size % ALLOC_BLOCK_SIZE) + 1;
	/* Need to make sure we've got that much space left. */
	Node **Indexer = &Attempt;
	UINT64 Increment = 0;
	while (*Indexer && ++Increment < BlockIncrement)
	{
		((*Indexer)->Used) = TRUE;
		Indexer = &((*Indexer)->Next);
	}
	Attempt->ChunkIncrement = BlockIncrement;
	InternalInitializeNode(Attempt, Attempt->Previous, Attempt->Next,
		(Attempt->Previous->NodeIndex) + 1);
	return Attempt->Area;
}