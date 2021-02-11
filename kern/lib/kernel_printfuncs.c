/*
Copyright (c) 2018, 2019, 2021, Brian Schnepp

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

#include <stdarg.h>

#include <feral/string.h>
#include <feral/stdtypes.h>
#include <feral/feralstatus.h>
#include <feral/kern/krnlfuncs.h>
#include <feral/kern/krnlfirmware.h>
#include <feral/kern/frlos.h>

/*
	Define internal kernel functions here. (hence 'krnlfun'.)
 */


static KrnlCharMap* CurrentCharMap = NULLPTR;
static KrnlFirmwareFunctions* BackingFunctions = NULLPTR;


FERALSTATUS KiUpdateFirmwareFunctions(
	KrnlFirmwareFunctions* Table, KrnlCharMap* CharMap)
{
	CurrentCharMap = CharMap;
	BackingFunctions = Table;
	return STATUS_SUCCESS;
}

FERALSTATUS KiPutChar(CHAR c)
{
	BackingFunctions->PutChar(c);
	return STATUS_SUCCESS;
}

FERALSTATUS KiBackspace(VOID)
{
	BackingFunctions->Backspace();
	return STATUS_SUCCESS;
}

/*
//Same as above but with a wide string.
FERALSTATUS KiGetWideStringLength(IN WSTRING, OUT* UINTN);
*/

#define PRINT_LINE_GENERIC() \
	UINT64 length; \
	FERALSTATUS FeralStatusError = KiGetStringLength(string, &length); \
	if (FeralStatusError != STATUS_SUCCESS) \
	{ \
		return FeralStatusError; \
	}

FERALSTATUS KiPrintLine(IN STRING string)
{
	PRINT_LINE_GENERIC();
	/* Ok, we call Println and use a black on white color set. */
	BackingFunctions->Println(string, length);
	return STATUS_SUCCESS;
}

FERALSTATUS KiPrint(STRING string)
{
	PRINT_LINE_GENERIC();
	for (UINT64 i = 0; i < length; ++i)
	{
		CHAR let = string[i];
		KiPutChar(let);
	}
	return STATUS_SUCCESS;
}


FERALSTATUS KiMoveCurrentPosition(UINT16 X, UINT16 Y)
{
	UNUSED(X);
	UNUSED(Y);
	return STATUS_UNSUPPORTED_OPERATION;
}

#if defined(__x86_64__)
#include <arch/x86_64/vga/vga.h>

/* Todo: aarch64 version(s). This breaks aarch64 port.*/

FERALSTATUS KiPrintGreyLine(STRING string)
{
	PRINT_LINE_GENERIC();
	VgaPrintln(VGA_LIGHT_GREY, VGA_BLACK, string, length);
	return STATUS_SUCCESS;
}

FERALSTATUS KiPrintWarnLine(STRING string)
{
	PRINT_LINE_GENERIC();
	VgaPrintln(VGA_LIGHT_BROWN, VGA_BLACK, string, length);
	return STATUS_SUCCESS;
}

#else

#define KiPrintWarnLine(a)
#define KiPrintGreyLine(a)

#endif
/* TODO: implement for other platforms. */

FERALSTATUS KiPrintFmt(const STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	/*
		Note that out of some laziness,
		we're not properly checking
		any datatypes here.

		You can pass a an int as a string
		and it'll run just fine... and never stop
		parsing. Not great.
	*/

	BOOL upState = FALSE;
	UINT8 repeatedCount = 0;

	UINT64 index = 0;
	for (CHAR cur = fmt[0]; cur != '\0'; cur = fmt[++index])
	{
		if (cur == '%' && !upState)
		{
			/* Push up */
			upState = TRUE;
		}
		else if (upState)
		{
			/*
				If this state is up, it means we're in a special
				format mode (ie, the % char was present and were
				not done parsing it.)
			 */
			if (cur == 's')
			{
				/* We interpret as a string. */
				STRING valistnext;
				valistnext = va_arg(args, STRING);
				KiPrint(valistnext);
			}
			else if (cur == 'i')
			{
				INT32 valistnext;
				valistnext = va_arg(args, INT32);
				/* Create a buffer to store in. Integer is never
				 * longer than 20, so... */
				CHAR buf[21] = {0};
				KiItoaSigned((UINT64)valistnext, buf);
				KiPrint(buf);
			}
			else if (cur == 'l')
			{
				INT64 valistnext;
				valistnext = va_arg(args, INT64);
				/* Create a buffer to store in. Integer is never
				 * longer than 20, so... */
				CHAR buf[21] = {0};
				KiItoaSigned((UINT64)valistnext, buf);
				KiPrint(buf);
			}
			else if (cur == 'u')
			{
				UINT64 valistnext;
				valistnext = va_arg(args, UINT64);
				/* Create a buffer to store in. Integer is never
				 * longer than 21, so... */
				CHAR buf[22] = {0};
				KiItoa(valistnext, buf);
				KiPrint(buf);
			}
			else if (cur == 'x')
			{
				UINT64 valistnext;
				valistnext = va_arg(args, UINT64);
				/* Create a buffer to store in. Integer is never
				 * longer than 16, so... */
				CHAR buf[17] = {0};
				KiItoaBase(valistnext, 16, buf);
				KiPrint(buf);
			}
			else if (cur == '%')
			{
				KiPrint("%");
			}
			else if (cur >= '0' && cur <= '9')
			{
				if (fmt[++index] != '\0')
				{
					/*  FIXME: fully support it (we only
					 * lead with 0-9, and don't count
					 * places.) */
					/* What are we repeating? */
					repeatedCount
						= (UINT8)(fmt[index] - '0');
					/* Go through it some times. */
					for (UINT8 c = 0; c < repeatedCount;
						c++)
					{
						VgaPutChar(cur);
					}
				}
			}
			/* What do you mean %llu is a thing? */
			upState = FALSE;
		}
		else if (cur >= ' ')
		{
			KiPutChar(cur);
		}
		else if (cur == '\n')
		{
			KiPrintLine("");
		}
		else if (cur == '\t')
		{
			/* Add 8 spaces. */
			for (int i = 0; i < 8; i++)
			{
				KiPrint(" ");
			}
		}
	}
	va_end(args);
	return STATUS_SUCCESS;
}