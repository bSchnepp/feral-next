/*
Copyright (c) 2018, Brian Schnepp

Permission is hereby granted, free of charge, to any person or organization 
obtaining  a copy of the software and accompanying documentation covered by 
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


/* 
	Define internal kernel functions here. (hence 'krnlfun'.) 
 */

#include <feral/string.h>
#include <feral/stdtypes.h>
#include <feral/feralstatus.h>
#include <feral/kern/frlos.h>
#include <feral/kern/ki.h>

#include <stdarg.h>

#if defined(__x86_64__)
#include <arch/x86_64/vga/vga.h>

#define PRINT_LINE_GENERIC()												\
	UINTN length;															\
	FERALSTATUS feralStatusError = KiGetStringLength(string, &length);		\
	if (!(feralStatusError == STATUS_SUCCESS))								\
	{																		\
		return feralStatusError;												\
	}

FERALSTATUS KiPrintLine(STRING string)
{
	PRINT_LINE_GENERIC();
	// Ok, we call VgaPrintln and use a black on white color set.
	VgaPrintln(VGA_WHITE, VGA_BLACK, string, length);
	return STATUS_SUCCESS;
}

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

FERALSTATUS KiPrint(STRING string)
{
	PRINT_LINE_GENERIC();
	for (int i = 0; i < length; i++)
	{
		VgaPutChar(string[i]);
	}
	return STATUS_SUCCESS;
}

FERALSTATUS KiPrintFmt(const STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
#if 0
	/* We'll print in 1024-character buffers at a time.*/
	CHAR buf[1024];
	
	UINT64 index = 0;
	
	BOOL lastWasFormat = FALSE;
	/* Go through everything in fmt. If we come across a %, we need to do something.*/
	for (const char* ch = fmt; *ch; ch++)
	{
		/* We need to check if this is a '%' */
		if (*ch == '%' && !lastWasFormat)
		{
			lastWasFormat = TRUE;
			continue;
		}
		
		#if 0
		VOID VgaStringEntry(VgaColorValue foreground, VgaColorValue background, CHAR* string, DWORD length, DWORD posx, DWORD posy)
		#endif
		/* VGA core is in deparate need of being refactored... */
		if (lastWasFormat)
		{
		switch (*ch)
		{
			case '%':
				VgaAutoEntry(VGA_WHITE, VGA_BLACK, '%');
				break;
			
			case 'i':
				VgaAutoEntry(VGA_WHITE, VGA_BLACK, *ch);
				break;
				
			default:
				VgaAutoEntry(VGA_WHITE, VGA_BLACK, *ch);
				break;
		}
		}
		else
		{
			VgaAutoEntry(VGA_WHITE, VGA_BLACK, *ch);
		}
	}
#endif

	CHAR buffer[1024];
	/* 
		For now, we have a bug where over 1024 characters causes problems.
		I'll get around to fixing it when we *really* need it.
	 */
	BOOL lastWasFormat = FALSE;
	UINT64 index = 0;
	for (UINT32 fmtindex = 0; fmtindex < 1024; fmtindex++)
	{
		CHAR c = fmt[fmtindex];
		if (c != '%' && !lastWasFormat)
		{
			buffer[index++] = c;
		}
		else if (c == '%' && !lastWasFormat)
		{
			lastWasFormat = TRUE;
		}
		else if (lastWasFormat)
		{
			if (c == '%')
			{
				buffer[index++] = '%';
				lastWasFormat = FALSE;
			}
			else if (c == 's')
			{
				STRING valistnext;
				valistnext = va_arg(args, STRING);
				UINT64 sublen = 0;
				while (valistnext[sublen] != '\0')
				{
					buffer[index++] = valistnext[sublen++];
				}
			}
		}
	}
	/*  We're gonna have double newlines for a bit (kludgy TODO hack) */
	buffer[1023] = '\0';
	KiPrintLine(buffer);
	
	va_end(args);
	return STATUS_SUCCESS;
}

#else

// TODO
#define KiPrintWarnLine(a)
#define KiPrintGreyLine(a)
#define KiPrintLine(a)

#endif
// TODO: implement for other platforms.







FERALSTATUS FrlCreateString(IN FERALSTRING* StringLocation, UINT64 Length, WSTRING Content)
{
	StringLocation->Length = Length;
	StringLocation->Content = Content;
	return STATUS_SUCCESS;
}

FERALSTATUS FrlDeleteString(IN FERALSTRING* String)
{
	//TODO
	return STATUS_SUCCESS;
}

FERALSTATUS FrlCloneString(IN FERALSTRING* Source, IN FERALSTRING* OutLocation)
{
	//TODO
	return STATUS_SUCCESS;
}
