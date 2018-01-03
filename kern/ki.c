/*
Copyright (c) 2018, Brian Schnepp

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


// Define internal kernel functions here. (hence 'krnlfun'.)

#include <feral/stdtypes.h>
#include <feral/feralstatus.h>
#include <feral/kern/ki.h>

// Start, end, size.
FERALSTATUS KiCopyMemory(IN VOID* Source, IN VOID* Dest, IN UINTN Amount)
{
	UINTN* Destination = ((UINTN*)Dest);
	UINTN* Src = ((UINTN*)Source);
	if ((Source == NULL) || (Dest == NULL))
	{
		return STATUS_INVALID_MEMORY_LOCATION;
	}
	for (UINTN i = 0; i < Amount; i++)
	{
		Destination[i] = Src[i];
	}
	return STATUS_SUCCESS;
}

//TODO: Implement the rest of this stuff.
#if 0
//First, Second, Size, Equal
FERALSTATUS KiCompareMemory(IN VOID*, IN VOID*, IN UINTN, OUT BOOL);

//Start, New location, size
FERALSTATUS KiMoveMemory(IN VOID*, IN CONST VOID*, IN UINTN);

//Where, with what, and how many UINTNs to set.
FERALSTATUS KiSetMemory(INOUT VOID*, IN UINTN, IN UINTN);

// Same as above, but with bytes.
FERALSTATUS KiSetMemoryBytes(INOUT VOID*, IN UINT8, IN UINTN);
#endif


FERALSTATUS KiGetStringLength(IN STRING String, OUT UINTN Length)
{
	if (String == NULL)
	{
		return STATUS_INVALID_MEMORY_LOCATION;
	}
	UINTN Len = 0;
	while (String[Len])
	{
		Len++;
	}
	Length = Len;
	return STATUS_SUCCESS;
}
/*
//Same as above but with a wide string.
FERALSTATUS KiGetWideStringLength(IN WSTRING, OUT UINTN);
*/

