/*
Copyright (c) 2018, 2019, 2020 Brian Schnepp

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

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>
#include <feral/handle.h>
#include <feral/kern/frlos.h>
#include <mm/mm.h>

#include <feral/kern/krnlfuncs.h>
#include <feral/kern/krnlbase.h>

#include <krnl.h>
#include <kern_ver.h>

#include <arch/processor.h>

typedef struct FeralVersionInfo
{
	UINT8 FeralVersionMajor;
	UINT8 FeralVersionMinor;
	UINT8 FeralVersionPatch;
} FeralVersionInfo;


static FeralVersionInfo VersionInfo;

/* 	
	This is the kernel's *real* entry point. 
	TODO: some mechanism for a Feral-specific bootloader to skip the 
	multiboot stuff and just load this.
*/
VOID KiSystemStartup(KrnlEnvironmentBlock *EnvBlock)
{
	/* First off, ensure we load all the drivers, 
	 * so we know what's going on. Use a couple prints to check for
	 * any regressions. 
	 */
	KiUpdateFirmwareFunctions(EnvBlock->FunctionTable, EnvBlock->CharMap);

	VersionInfo.FeralVersionMajor = FERAL_VERSION_MAJOR;
	VersionInfo.FeralVersionMinor = FERAL_VERSION_MINOR;
	VersionInfo.FeralVersionPatch = FERAL_VERSION_PATCH;

	KiPrintFmt("\nStarting Feral Kernel \"%s\" Version %01u.%01u.%01u\n",
		FERAL_VERSION_SHORT,
		VersionInfo.FeralVersionMajor,
		VersionInfo.FeralVersionMinor,
		VersionInfo.FeralVersionPatch);

	KiPrintLine("Copyright (c) 2018-2020, Brian Schnepp");
	KiPrintFmt("Booted using %s\n",
		EnvBlock->FunctionTable->GetFirmwareName());
	KiPrintFmt("%s\n", "Preparing execution environment...");

	KiStartupSystem(FERAL_SUBSYSTEM_ARCH_SPECIFIC);
	KiStartupSystem(FERAL_SUBSYSTEM_MEMORY_MANAGEMENT);

	/* Only load drivers *after* base system initializtion. */
	KiPrintFmt("Loading all drivers...\n");
	FERALSTATUS KiLoadAllDrivers(VOID);

	/* These are macroed away at release builds.  
	 * They're eliminated at build time.
	 */
	KiDebugPrint("INIT Reached here.");
	/* 
		TODO: Call up KiStartupProcessor for each processor.
		Each processor should have it's x87 enabled, so we can do SSE 
		stuff in usermode. 
	 */
}

/* UINT64 just in case there's some crazy 6 billion core server one day. */
VOID KiStartupProcessor(UINT64 ProcessorNumber)
{
	/*  Create a new stack for this core. */
}


FERALSTATUS KeBootstrapSystem(VOID)
{
	/* Bootstrap process to actually get to user mode. */
	return STATUS_SUCCESS;
}
