/*
Copyright (c) 2018, 2019, Brian Schnepp

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
#include <feral/kern/frlos.h>
#include <feral/kern/krnlfuncs.h>

#include <feral/string.h>
#include <feral/feraluser.h>

#include "sec.h"

#if defined(__x86_64__)
#include <arch/x86_64/vga/vga.h>
#endif

typedef struct
{
	FERALSTRING UserMapping;
	WSTRING RootDir;
}SeUserContext;


typedef struct
{
	UINT64 IdentifierPrimary;
	UINT64 IdentifierSeconday;

	SeUserContext UserContext;
		
}SeSecurityContext;


/* TODO: Refactor to have return as FERALSTATUS, and an INOUT parameter for the string. */
STRING KiGetErrorType(IN FERALSTATUS Status)
{
	if (Status == STATUS_STACK_GUARD_VIOLATION)
	{
		return "STATUS_STACK_GUARD_VIOLATION";
	} else {
		/* TODO */
	}
}

FERALSTATUS KiStopError(IN FERALSTATUS Status)
{
#if defined(__x86_64__)
	KiBlankVgaScreen(25, 80, VGA_BLUE);
	char* errorMsg = "A problem has been detected and Feral has shutdown to prevent further damage.";
	UINTN length = 0;
	KiGetStringLength(errorMsg, &length);
	VgaPrintln(VGA_WHITE, VGA_BLUE, errorMsg, length);
#endif
	KiPrintFmt("If this is the first time this error has occured, try restarting your computer.\n");
	
	KiPrintLine("");
	
	KiPrintFmt("If the error persists, contact tech support and provide the following info:\n");
	KiPrintFmt("Bug Check: 0x%x: %s\n", Status, KiGetErrorType(Status));
	
	/* TODO: We haven't implemented these function quite just yet, so uncomment when we do. */
	/* KiPrintFmt("Bug Check occured at epoch time %l\n", (KeGetCurrentTime())); */
	/* KiPrintFmt("System uptime is %l\n", (KeGetCurrentUptime() / 1000)); */
	/* KiPrintFmt("System had %l tasks running\n", (KeGetTaskCount())); */
	
	/* KiPrintFmt("Feral will reboot in 15 seconds: log contents will be written to A:/System/Logs.\n"); */
	for (;;)
	{
		/* Hang (for now) */
	}
	
	return STATUS_ERROR;
}

__attribute__((noreturn))
VOID __stack_chk_fail(void)
{
	KiStopError(STATUS_STACK_GUARD_VIOLATION);
	for (;;)
	{
		/* Hang (for now) */
	}
}