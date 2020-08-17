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

#ifndef _FERAL_PROCESSOR_H_
#define _FERAL_PROCESSOR_H_

#include <feral/stdtypes.h>
#include <feral/feralstatus.h>

typedef struct THREAD_INFO
{
	UINT64 ThreadId; /* On the physical processor, what thread are we? (ie,
			    thread 0, 1, 2, 3 for 4-way SMT) */

	UINT64 TaskCount;
	/* TODO: task list of this processor/ */

	BOOL ThreadEnabled; /* Is this thread currently working? */
};

typedef struct PROCESSOR_INFO
{
	UINTN ProcessorNumber;
	BOOL DedicatedToApplication; /* Did we dedicate this processor to one
					application? 	*/
	BOOL SMTSupported; /* Does this core support SMT?
			    */
	BOOL SVMSupported; /* Supported by Pacifica VM extensions? ?
			    */
} PROCESSOR_INFO;

typedef struct SYSTEM_INFO
{
	UINT16 Arch;
	UINT64 ProductNameLength;
	CHAR* ProductName;

	UINTN ProcessorCount;
	PROCESSOR_INFO* Processors;
} SYSTEM_INFO;

#define PROCESSOR_ARCH_IA32 \
	0x014C /* We don't _really_ support 32-bit X86, but we might want to \
		  later.	*/
#define PROCESSOR_ARCH_X86_64 0x8664
#define PROCESSOR_ARCH_IA64 0x0200
#define PROCESSOR_ARCH_AARCH64 0xAA64
#define PROCESSOR_ARCH_AARCH32 \
	0xAA32 /* Only support those weird versions which can do more than 4GB \
		  of RAM.	*/
#define PROCESSOR_ARCH_RISCV32 \
	0x5032 /* Only support if we _really have to_.	*/
#define PROCESSOR_ARCH_RISCV64 0x5064 /* We expect RVGC64.	*/
#define PROCESSOR_ARCH_RISCV128 0x5128



#endif


#if 0
#endif
