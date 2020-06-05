/*
Copyright (c) 2020, Brian Schnepp

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
#include <arch/x86_64/mm/pageflags.h>

#include <mm/mm.h>
#include <feral/kern/krnlfuncs.h>

BOOL CheckIfMapped(PageMapEntry *Entry)
{
	return (Entry->PresentFlag);
}

UINT_PTR ConvertPageEntryToAddress(PageMapEntry *Entry)
{
	UINT64 UnhandledAddress = (Entry->Address << 12);
	if (UnhandledAddress >> 47)
	{
		/* Higher half address... mirror bit 48. */
		UnhandledAddress |= 0xFFFF000000000000;
	}
	return (UINT_PTR)(UnhandledAddress);
}

/**
 * Finds the page levels required to allocate a specific, 4KiB aligned
 * address. Initial address is assumed to be aligned at 4KiB.
 * @author Brian Schnepp
 * @param Address The physical address to look up allocation for
 * @param Levels An array of 4 UINT16s which, from left to right,
 * will be written to with the order of level 4 page table to level 1
 * page table. In other words, the the most signifigant entry is the
 * first in the table.
 * @return STATUS_SUCCESS if valid
 */
FERALSTATUS x86FindPageLevels(UINT64 Address, IN OUT UINT16 *Levels)
{
	Levels[0] = (Address >> 12) & 0x1FF;
	Levels[1] = (Address >> 21) & 0x1FF;
	Levels[2] = (Address >> 30) & 0x1FF;
	Levels[3] = (Address >> 39) & 0x1FF;
	return STATUS_SUCCESS;
}

/**
 * Maps an address given a handle to the PML4, in virtual memory,
 * to a physical address and a desired virtual address.
 * A physical address can be bound to several different virtual
 * addresses, but a virtual address cannot be unbound with this function.
 * 
 * @author Brian Schnepp
 * 
 * @param PML4 The PML4 to reference, which is usually retrived from the CR3
 * register
 * @param Physical The desired physical address to map
 * @param Virtual The desired virtual address to map the Physical address to
 * 
 * @return STATUS_SUCCESS on success, STATUS_MEMORY_PAGE_FAILURE if the
 * virtual address is already in use
 */
FERALSTATUS x86MapAddress(PageMapEntry *PML4, UINT_PTR Physical, UINT_PTR Virtual)
{
	/* Shift over 12 bytes, to get to level 4 table immediately. */
	UINT64 Addr = Physical >> 12;
	UINT16 Bitmask = 0xFFF; /* For now, only do 4096 byte pages. */

	UINT16 PageLevels[4];
	FERALSTATUS Err = x86FindPageLevels(Virtual, PageLevels);

	if (Err != STATUS_SUCCESS)
	{
		return Err;
	}

	/* TODO: Consider rolling this into a simple loop. */

	/* From the PML4, we need to find the pdpe entry. */
	PageMapEntry *PDP = NULLPTR;
	if (PML4[PageLevels[0]].PresentFlag)
	{
		PDP = (PageMapEntry*)(PML4[PageLevels[0]].Address << 12);
	} else {
		/* Create new page entry */
		PDP = (PageMapEntry*)MmKernelMalloc(sizeof(PageMapEntry) * 512);
		KiSetMemoryBytes(PDP, 0, sizeof(PageMapEntry) * 512);

		/* Add in the entry */
		PML4[PageLevels[0]].Address = (((UINT64)PDP) >> 12);
		PML4[PageLevels[0]].PresentFlag = TRUE;
		PML4[PageLevels[0]].WritableFlag = TRUE;
	}

	/* Again for level 3... */
	PageMapEntry *PD = NULLPTR;
	if (PDP[PageLevels[1]].PresentFlag)
	{
		PD = (PageMapEntry*)(PDP[PageLevels[1]].Address << 12);
	} else {
		/* Create new page entry */
		PD = (PageMapEntry*)MmKernelMalloc(sizeof(PageMapEntry) * 512);
		KiSetMemoryBytes(PD, 0, sizeof(PageMapEntry) * 512);

		/* Add in the entry */
		PDP[PageLevels[1]].Address = (((UINT64)PD) >> 12);
		PDP[PageLevels[1]].PresentFlag = TRUE;
		PDP[PageLevels[1]].WritableFlag = TRUE;
	}

	/* Again for level 2... */
	PageMapEntry *PT = NULLPTR;
	if (PD[PageLevels[2]].PresentFlag)
	{
		PT = (PageMapEntry*)(PD[PageLevels[2]].Address << 12);
	} else {
		/* Create new page entry */
		PT = (PageMapEntry*)MmKernelMalloc(sizeof(PageMapEntry) * 512);
		KiSetMemoryBytes(PT, 0, sizeof(PageMapEntry) * 512);

		/* Add in the entry */
		PD[PageLevels[2]].Address = (((UINT64)PT) >> 12);
		PD[PageLevels[2]].PresentFlag = TRUE;
		PD[PageLevels[2]].WritableFlag = TRUE;
	}

	/* Again for level 1... */
	if (PT[PageLevels[3]].PresentFlag)
	{
		return STATUS_MEMORY_PAGE_FAILURE;
	} else {
		PT[PageLevels[3]].Address = (((UINT64)Physical) >> 12);
		PT[PageLevels[3]].PresentFlag = TRUE;
		PT[PageLevels[3]].WritableFlag = TRUE;
	}

	return STATUS_SUCCESS;
}

/**
 * Unmaps an address given a handle to the PML4, in virtual memory,
 * to a physical address and a desired virtual address.
 * Note that a virtual address has a many-to-one relationship with a physical
 * address, and unbinding a particular virtual address may not make a given
 * physical address unreachable if it happens to be mapped to more than one
 * location.
 * 
 * @author Brian Schnepp
 * 
 * @param PML4 The PML4 to reference, which is usually retrived from the CR3
 * register
 * @param Virtual The desired virtual address to unmap
 * 
 * @return STATUS_SUCCESS on success, STATUS_MEMORY_PAGE_FAILURE if the
 * virtual address was not already in use
 */
FERALSTATUS x86UnmapAddress(PageMapEntry *PML4, UINT_PTR Virtual)
{
	UINT16 Bitmask = 0xFFF; /* For now, only do 4096 byte pages. */

	UINT16 PageLevels[4];
	FERALSTATUS Err = x86FindPageLevels(Virtual, PageLevels);

	if (Err != STATUS_SUCCESS)
	{
		return Err;
	}

	/* TODO: Consider rolling this into a simple loop. */

	/* From the PML4, we need to find the pdpe entry. */
	PageMapEntry *PDP = NULLPTR;
	if (PML4[PageLevels[0]].PresentFlag)
	{
		PDP = (PageMapEntry*)(PML4[PageLevels[0]].Address << 12);
	} else {
		return STATUS_MEMORY_PAGE_FAILURE;
	}

	/* Again for level 3... */
	PageMapEntry *PD = NULLPTR;
	if (PDP[PageLevels[1]].PresentFlag)
	{
		PD = (PageMapEntry*)(PDP[PageLevels[1]].Address << 12);
	} else {
		return STATUS_MEMORY_PAGE_FAILURE;
	}

	/* Again for level 2... */
	PageMapEntry *PT = NULLPTR;
	if (PD[PageLevels[2]].PresentFlag)
	{
		PT = (PageMapEntry*)(PD[PageLevels[2]].Address << 12);
	} else {
		return STATUS_MEMORY_PAGE_FAILURE;
	}

	/* Again for level 1... */
	if (PT[PageLevels[3]].PresentFlag)
	{
		PT[PageLevels[3]].PresentFlag = FALSE;
		PT[PageLevels[3]].WritableFlag = FALSE;
		PT[PageLevels[3]].Address = 0x00;
	} else {
		return STATUS_MEMORY_PAGE_FAILURE;
	}

	return STATUS_SUCCESS;
}
