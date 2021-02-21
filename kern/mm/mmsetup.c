/*
Copyright (c) 2019, 2020, 2021, Brian Schnepp

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
#include <feral/feralstatus.h>

#include <krnl.h>
#include <feral/kern/frlos.h>
#include <feral/kern/krnlfuncs.h>

#include <mm/mm.h>
#include <mm/page.h>

/* These are done by the linker. */
extern UINTN kern_start;
extern UINTN kern_end;

/* Convert the addresses into integers for easier comparison. */
static UINT_PTR kernel_start = (UINT_PTR)&kern_start;
/* TODO: Use kernel_end? */


typedef struct MemoryManagementState
{
	/* We'll need to store how physical address are allocated. */
	MmPhysicalAllocationInfo *pAllocInfo;

	/* We'll need the log of the page size divided by 8 to
	 * find the size needed for the whole thing, along with
	 * the multiplication of the total amount of memory times
	 * that number.
	 */
	UINT8 *BitmaskUsedFrames;
	UINT_PTR MaxPAddr;
} MemoryManagementState;

/* Not the best thing ever, but... */
static MemoryManagementState MmState;


/* TODO: Move to other source file. */
FERALSTATUS FERALAPI KiInitializeMemMgr(MmCreateInfo *info)
{
	/* Association the pAllocInfo first. */
	MmState.pAllocInfo = info->pPhysicalAllocationInfo;

	/* Look through the memory locations. Place at PMM in first place
	   I guess for now.
	 */
	UINT64 TotalSystemMemory = 0;
	UINT_PTR PmmLocation = 0;
	UINT_PTR MaximumPAddr = 0;

	for (UINT64 n = 0; n < MmState.pAllocInfo->MemoryAreaRangeCount; ++n)
	{
		MmMemoryRange range = MmState.pAllocInfo->Ranges[n];
		/* Add the size in to the total. */
		TotalSystemMemory += range.Size;

		/* What's the end address here? Is it bigger? */
		if (range.End > MaximumPAddr)
		{
			MaximumPAddr = range.End;
		}
	}
	MmState.MaxPAddr = MaximumPAddr;

	/* The PMM is a bitmap. Each bit represents one of FrameSize.
	 * As such, the size is the total memory / (8 * FrameSize).
	 * The kernel also passed us a nice safe place to put the
	 * PMM, so that should be used as well.
	 */
	PmmLocation = ((UINT_PTR)(info->SafePMMArea));
	UINT_PTR FrameSize = info->pPhysicalAllocationInfo->FrameSize;
	UINT64 BufferSize = MmState.MaxPAddr / (8 * FrameSize);

	/* Take our location from before, and use it accordingly. */
	UINT8 *PmmBitmap = (UINT8 *)(PmmLocation + FrameSize);
	KiSetMemoryBytes(PmmBitmap, 0, BufferSize);
	MmState.BitmaskUsedFrames = KERN_PHYS_TO_VIRT(PmmBitmap);

	/* Mark everything as in use. */
	for (UINT_PTR Index = kernel_start; Index < (PmmLocation) + BufferSize;
		Index += FrameSize)
	{
		VALIDATE_SUCCESS(SetMemoryAlreadyInUse(Index, TRUE));
	}

	/* Start the heap stuff up. No SMP support yet, so 1 hart. */
	/* 4M heap should be enough for now? */
	const UINT64 HeapSize = (1024 * 1024 * 8);
	UINT_PTR HeapAddr = ((PmmLocation) + BufferSize);
	HeapAddr += FrameSize;
	MmCreateAllocatorState(1, (void *)HeapAddr, HeapSize);
	for (UINT_PTR Addr = HeapAddr; Addr < (HeapSize);
		Addr += MmState.pAllocInfo->FrameSize)
	{
		VALIDATE_SUCCESS(SetMemoryAlreadyInUse(Addr, TRUE));
	}
	return STATUS_SUCCESS;
}

FERALSTATUS GetMemoryAlreadyInUse(UINT_PTR Location, BOOL *Status)
{
	/* Can't talk about a physical address we don't have. */
	if (Location > MmState.MaxPAddr)
	{
		return STATUS_INVALID_MEMORY_LOCATION;
	}
	/* Try to get everything before this cell out of the way. */
	/* Each bit represents MmState.pAllocInfo->FrameSize bytes...
	  Mult. this by 8 for per byte. Look until expected address > Location.
	*/


	UINT_PTR FrameSize = MmState.pAllocInfo->FrameSize;
	UINT_PTR BaseAddr = (Location / (8 * FrameSize));

	/* BaseAddr will now be at the *byte* we want. */
	/* We take the formula (FrameSize) % 8 in order to find the bit
	    to check.
	  */
	UINT8 ContainingByte = MmState.BitmaskUsedFrames[BaseAddr];
	UINT8 BitNum = (Location / FrameSize) % 8;
	*Status = (ContainingByte >> BitNum) & 0x01;
	return STATUS_SUCCESS;
}

FERALSTATUS SetMemoryAlreadyInUse(UINT_PTR Location, BOOL Status)
{
	/* Can't talk about a physical address we don't have. */
	if (Location > MmState.MaxPAddr)
	{
		return STATUS_INVALID_MEMORY_LOCATION;
	}
	/* Try to get everything before this cell out of the way. */
	/* Each bit represents MmState.pAllocInfo->FrameSize bytes...
	  Mult. this by 8 for per byte. Look until expected address > Location.
	*/

	UINT_PTR FrameSize = MmState.pAllocInfo->FrameSize;
	UINT_PTR BaseAddr = (Location / (8 * FrameSize));

	/* BaseAddr will now be at the *byte* we want. */
	/* We take the formula (FrameSize) % 8 in order to find the bit
	    to check.
	  */
	UINT8 ContainingByte = MmState.BitmaskUsedFrames[BaseAddr];
	UINT8 BitNum = (Location / FrameSize) % 8;
	BOOL InUse = (ContainingByte >> BitNum) & 0x01;

	if (InUse && !Status)
	{
		/* Free it */
		MmState.BitmaskUsedFrames[BaseAddr]
			= (ContainingByte) & ((255) ^ (1 << BitNum));
		return STATUS_SUCCESS;
	}
	else if (!InUse && Status)
	{
		/* Allocate it. */
		MmState.BitmaskUsedFrames[BaseAddr]
			= (ContainingByte) | (1 << BitNum);
		return STATUS_SUCCESS;
	}
	else
	{
		return STATUS_MEMORY_ACCESS_VIOLATION;
	}
}

/**
 * @brief Looks for a free physical page in the current physical memory
 * manager, which may be allocated freely in future operations.
 *
 * @author Brian Schnepp
 * @param Location The resulting location that should be written to when
 * a page is found. If no free page is found, it is set to 0.
 *
 * @return STATUS_SUCCESS on a free page located, STATUS_MEMORY_PAGE_FAILURE
 * if none was found.
 */
FERALSTATUS MmLookupFreeMemoryPage(OUT UINT_PTR *Location)
{
	UNUSED(Location);

	for (UINT64 PageBlock = 0; PageBlock < MmState.MaxPAddr; PageBlock++)
	{
		for (UINT8 Bit = 0; Bit < 8; ++Bit)
		{
			if (((MmState.BitmaskUsedFrames[PageBlock])
				    & (1 << Bit))
				== 0)
			{
				/* A free page was found. */
				*Location = (MmState.pAllocInfo->FrameSize
						    * PageBlock * 8)
					    + (Bit
						    * MmState.pAllocInfo
							      ->FrameSize);
				return STATUS_SUCCESS;
			}
		}
	}

	*Location = 0;
	return STATUS_MEMORY_PAGE_FAILURE;
}