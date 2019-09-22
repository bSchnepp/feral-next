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
#include <mm/heap.h>
#include <mm/mm.h>

AllocatorState MmCreateAllocatorState(UINT64 NumArenas, VOID *HeapArea)
{
	AllocatorState State;
	
	State.sType = MM_STRUCTURE_TYPE_HEAP_ALLOCATOR;
	State.pNext = (void*)(0);
	
	Arena *ArenaStart = (Arena*)(HeapArea);
	for (UINT64 Counter = 0; Counter < NumArenas; ++Counter)
	{
		/* Write to that area an Arena... */
		Arena Current;
		Current.Lock = 0;
		Current.ThreadIndex = 0;
		Current.Size = 0;
		Current.Area = (void*)(0);
		Current.Root = (void*)(0);
		/* TODO: Properly setup */
		ArenaStart[Counter] = Current;
	}
	return State;
}
