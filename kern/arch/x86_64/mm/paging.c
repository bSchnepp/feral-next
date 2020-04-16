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

#include <arch/x86_64/mm/pageflags.h>


BOOL CheckIfMapped(PageMapEntry *Entry)
{
	return (Entry->Present);
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

FERALSTATUS MapAddress(PageMapEntry *PML4, UINT_PTR Physical, UINT_PTR Virtual)
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

	PageMapEntry* Level3Table = (PageMapEntry*)(PML4[PageLevels[3]);
	PageMapEntry* Level2Table = (PageMapEntry*)(
		ConvertPageEntryToAddress(Level3Table)[PageLevels[2]
	);
	PageMapEntry* Level1Table = (PageMapEntry*)(
		ConvertPageEntryToAddress(Level2Table)[PageLevels[1]
	);

	PageMapEntry *FinalLevel = (PageMapEntry *)(ConvertPageEntryToAddress(Level1Table)[PageLevels0]);

	return STATUS_SUCCESS;
}


FERALSTATUS CreateNewPageTable(
