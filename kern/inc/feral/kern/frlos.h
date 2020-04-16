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

#ifndef _FERAL_FRLOS_H_
#define _FERAL_FRLOS_H_

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>

/* All kernel functions should return a FERALSTATUS. */
FERALSTATUS KiPrintLine(STRING string);
FERALSTATUS KiPrintGreyLine(STRING string);
FERALSTATUS KiPrintWarnLine(STRING string);
FERALSTATUS KiPrint(STRING string);
FERALSTATUS KiPrintFmt(const STRING fmt, ...);
FERALSTATUS KiMoveCurrentPosition(UINT16 X, UINT16 Y);

/*
	 Security is always turned on when kernel initializes. Why would you want to disable it? 
	 (okay, if you want a vulnerable kernel for playing with that sort of thing, we'll leave a compile-time macro...)
*/

typedef enum KiSubsystemIdentifier
{
	FERAL_SUBSYSTEM_MEMORY_MANAGEMENT,
	FERAL_SUBSYSTEM_ARCH_SPECIFIC,
	FERAL_SUBSYSTEM_FILESYSTEM_MANAGEMENT,
	FERAL_SUBSYSTEM_OBJECT_MANAGEMENT,
	FERAL_SUBSYSTEM_VIDEO_MANAGEMENT,
	FERAL_SUBSYSTEM_POWER_MANAGEMENT,
	FERAL_SUBSYSTEM_PROCESS_MANAGEMENT, /* We can disable this if we want something like a thin client, and only one program (and one thread) per core.*/

	FERAL_SUBSYSTEM_NETWORK_STACK,
	FERAL_SUBSYSTEM_EMULATION_STACK, /* CPU and 'foreign OS' emulation, including dynarec compiler(s). */
	FERAL_SUBSYSTEM_AUDIO_STACK,
	FERAL_SUBSYSTEM_INPUT_STACK,
} KiSubsystemIdentifier;

/* Bring up a system needed for the kernel. */
FERALSTATUS KiStartupSystem(KiSubsystemIdentifier subsystem);

#ifdef KERN_DEBUG

#ifndef _NO_SERIAL_DEBUG_PRINT_

/* hack for serial */
#ifndef COM1_PORT
#define COM1_PORT 0x3F8
#endif

extern VOID SerialSend(UINT16 Port, CHAR c);

#define SERIAL_PUTCHAR(x) SerialSend(COM1_PORT, x)
#else
#define SERIAL_PUTCHAR(x) /* Nothing! */
#endif

inline VOID SerialPrint(const char *X)
{
	UINT64 Index = 0;
	while (X[Index] != '\0')
	{
		SERIAL_PUTCHAR(X[Index]);
		Index++;
	}
}

static FERALSTATUS KiDebugPrint(STRING string)
{
	KiPrintWarnLine(string);
	SerialPrint(string);
	SerialPrint("\n");
	return KiPrintFmt("\n");
}
#else
#define KiDebugPrint(x)
#endif

#endif
