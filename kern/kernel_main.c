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

#include <stdint.h>

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>
#include <feral/handle.h>
#include <feral/kern/frlos.h>

#if defined(__x86_64__)
#include <arch/x86_64/vga/vga.h>
#include <arch/x86_64/cpuio.h>
#include <arch/x86_64/cpufuncs.h>
#endif

#include <kern_ver.h>

#if defined(__x86_64__) || defined(__i386__)
#ifndef FERAL_BUILD_STANDALONE_UEFI_
#include "multiboot/multiboot2.h"
#endif
#endif

#include <feral/boot/kibootstruct.h>
#include <feral/kern/krnlfuncs.h>

#include <krnl.h>


#include <arch/processor.h>

static SYSTEM_INFO SystemInfo;
static UINT8 FeralVersionMajor;
static UINT8 FeralVersionMinor;
static UINT8 FeralVersionPatch;

/* 
	TODO: comments are bad, and all of this is C99 mess, and needs to be redone in C89 comment style.
	 We can bother with that later, so if you want this running on your "Actual Potato computer", just compile with GNU extensions or something. 
 */

#if defined(KERN_DEBUG)
#define ILOVEBEAR18 (1)	/* 
							flag for experimental kernel features, intentionally something strange such that someone 
							doesn't see this in an automated flag thing and just turns it on without knowing why. 
						*/
#endif

static WSTRING RootFsLocation;	/* Where is the root? This should normally be A:/, but it can be anywhere. */


/* FERAL initialization follows a few basic steps: */
	/*
		[we'll need to rewrite this, since a lot of this is reworked into the 'master control process'.]
		The first process is always "Subsystem manager". (SSMGR.PRO), this handles dynamic linking and is essential for all non-native programs.
			(all userspace programs, unless they ship with the rest of the OS, should be a "non-native" (link with a libos) program.
			(all non-native programs inherit from this one, ala *NIX. This does the primary job of "init".)

		The second is always WAYLOGON.PRO, which should handle user authentification in all cases, even if it's for our sort-of-SMB-slash-NFS stuff.
			(this establishes a user object in the kernel and gets out some unique identifier so as to associate users with actions)

		The third is always SESSMGR.PRO, which should handle actual user sessions (created by waylogon), and handle environment variables, initialize the DCL-like shell,
		do some important stuff with devices (kind of the same as how *NIX create device files like /dev/sda, /dev/hda, /dev/disk0s1, etc.),
			We borrow DOS convention of the names (rather than *NIX-style) The names are not *exact*, only similar. 
			Note the underlying philosophy is "everything is a resource, not necessarilly a file".
			ie, \\.\Devices/CON1
			    \\.\Devices/CON2
			    \\.\Devices/CON3
			    \\.\Devices/NULL
			    \\.\Devices/COM1
			    \\.\Devices/VGA1		(even if it's actually HDMI or DP or whatever, we refer to it as a 'VGA device')
			    \\.\Devices/AUX
			    \\.\Devices/IDLE
			    \\.\Devices/KEYB1
			    \\.\Devices/MOUSE1
				(etc)
				
			Network resources would be accessed via:
			\\.\0x93252290D23F0B5E84B58AC3F8C4CD62D630C525B91FB962B14C7B63E5DFD2CDBD273C09886A51AE4EA6F806C8A3AE55FB5C60553D7121A6CBC304C3B22916BDD63F5AF36688728A1458F7763320B2FB96972A33644A401431E0A6024370FC/index.html
			for example, where without specifying 'ipv4::', 'ipv6::', '9p::', etc. before the address, we assume RENEGADE.
			Obviously we'll implement a domain service so that mass of numbers isn't required.

			It also assigns drive letters (the system default 'A', secondary drives get some way of assigning a letter to them, 
			(usually in alphabetical order (A:/,, B:/, ...,  AB:/, ...,  AZ:/, AAA:/, AAB:/, and so on)
	 */

static CHAR* cpu_vendor_msg = "CPU Vendor: ";
static SYSTEM_INFO KernelSystemInfo = {};


/* TODO: Remove the "enormous ego" out of this.

	(after all, one of the reasons why I want to make Feral after all is all the little things that just add up with everything else out there,
	always missing one thing, or just could be done better, or could exist. Also because I really like building everything from the ground up.
	Really bad not-invented-here I guess.)
*/
/* 
	Things to do in order:
	Memory management subsystems, RAMFS filesystem
	Simple filesystem (ext2 is enough, 8.3 FAT would be OK to also support (possibly even needed).)
	Create libc, get C++/Rust stuff working on Waypoint. (We would like to make it very easy to create GUI apps for Waypoint, after all.)
	Get a stable userspace up and running. Essentially the syscall table should be similar-ish to Linux, so most assembly programs can be ported over fine.	
		Consider modifying architecture to figure out a way to have syscall tables be built by the libos...? (ie, NIX subsystem pretends to be Linux with a few Mach-isms, WINE-on-Feral pretends to be NTOS 10.0)
			(TODO: If we do this, figure out ways to deal with particularly annoying software that intentionally uses bugs or implementation details in order to work correctly...? This is easier to deal with for a Linux subsystem, but RedmondOS isn't just in 
			some git repo anyone can just clone.)
	Port LLVM/Clang, consider GCC/GAS port, we'd like native ports to the toolchain we really need, but it's fine if we don't (we're going to write a Linux emulation subsystem anyway.)
	Port LLD, YASM, and the rest of the toolchain. Possibly port a command-line text editor, or just write one from scratch. No GUI, so FreedomEdit isn't possible.
	Consider porting Python, golang, and for fun, get COBOL running on Feral.

	Work on being self-hosting: Feral needs to compile on Feral.
	Create a generic, VESA-compatible desktop environment. Assume legacy GPUs don't exist, and we're eventually going to need to support EFI stuff.
	Network driver for whatever NIC I happen to have lying around. Look for supporting whatever wifi chip is in my laptop too. (Hopefully an easy one.) Most of mine are Chipzilla (or clones), but not all of them.
	Create a vega10 GPU driver, get Vulkan-based desktop ready. (we do not care about vega11 or vega20 just yet, primarilly because I dont have the hardware, we *do* care about vega12 though.)
	We don't care about green GPUs at all. It's much too hard to support, overly complex, and no official reference drivers to peek at when lost at what to do.
	Create an alternative to the LiveCD tools we need currently, create a bootloader that understands (U)EFI. We implement UEFI ourselves, because tangling with inconsistently applied BSD license 
	is too much effort (not some easy shell script, and at that, I probably won't use bash on Waypoint, but maybe something more Virtual Memory System-y. We're aiming to not be *NIX anyway, so.).
	Implement Multiboot 2 support, purge old bootloader.
	New filesystem focused on getting as fast read times as possible on files (eliminate file fragmentation by not allowing fragmentation?, yes, more writes, but avoids non-sequential reads.)
			(for hard drives, specifically being sequential might actually be a bad thing because of the spinning. I wouldn't know though.)
	Add some random hypervisor capability just for fun or something (basically kvm).
	Get a working Vulkan driver for the vega10 GPU (Do some magic to port AMDVLK (is this possible?) and/or RADV or something, then just modify them as needed?).
	Port some open source games over, see if we can get it to outperform some other OSes just by running on Feral Waypoint. (specifically, everything we can from the 90s)
	We probably can't outperform Linux, but we'd better outperform some other desktop operating systems...
		(we're *always* 20-30 years late so we'd better catch up........). Linux tends to outperform RedmondOS (esp. CPU efficiency: TR4's 2990WX nearly double performance in many workloads) anyway. (Otherwise we'll *never* be taken seriously, instead of just "probably won't be")
	???

	Since we don't want to support anything older than the vega10 GPU, we should just cut out the unnecessary parts of whatever drivers we do adapt.
	(Those are too old, and by the time this becomes usable as a serious OS, those would be *long* obsolete anyway.)
	Migrate VCS server from ext4/btrfs/zfs to FeralFS. (again, dogfooding!!!!) Port FeralFS to BSD and Linux. The idea is that if it's bad, we'll fix it before bad things happen.
		Better put,  "We don't like garbage, so when we use garbage, we make garbage not garbage because otherwise we'd be using garbage, and we don't like garbage."
	
	Port mesa or get some graphics stack running atop Feral's existing driver structure, then fork Firefox and use it as system's web browser (and/or replace Gecko, move JS engine to ChakraCore, ...?).
	Hope a ReactOS compatibility layer falls out of the sky, so we can run some stuff, benchmark it, etc. The main thing is that we want at least 5% performance improvement over RedmondOS wherever we can get it.
	
	Consider C# on Feral, port WinForms and all that whole stuff. (Mainly, porting Mono)
	
	
	__before any full 0.0.1 release, ensure and triple-check that we've removed __all__ trademarked phrases in the kernel. We don't want to put up with the legal nonsense.
 */


/* TODO */
#if defined(__aarch64__)
FERALSTATUS KiPrintLine()
{
}

FERALSTATUS KiPrintWarnLine()
{
}
#endif

/* This is the kernel's *real* entry point. TODO: some mechanism for a Feral-specific bootloader to skip the multiboot stuff and just load this.*/
VOID KiSystemStartup(VOID)
{
	/*
	//TODO...
	//We need to look for every core on the system (We should expect 8, as we're expecting to run on a ZEN 1700X CPU. I will probably upgrade to something on socket TR4 though.)
	//As such, we need to run a function called HalInitializeProcessor (for the remaining 7 cores... for playing with a 1950X eventually, 15 remaining cores...)
	// I do now have a 1950X (turns out you don't need $1500 to fall out of the sky if you wait long enough after all), so we can build the kernel VERY VERY FAST and have plenty of Zen cores to run in VMs with.
	//And then call KiInitializeKernel as needed. We'll also have to use SMT when we can.

	//SMP support would also be nice, but I can't really imagine any desktop motherboards supporting a dual-CPU configuration. (though this would be AWESOME! And it doubles as a heater!)
	// We may need to support SMP anyway, ie, on X399, since the CPU on that platform is *technically* 2 - 4 "CPUs" put together on a single package (with each of those CPUs *technically* being "2 CPUs").
	// Better put, we have 4 cores per CCX. We have 2 CCXes per die, and we have 2 dies, so 4 * 2 * 2 = 8 * 2 = 16 cores. Each core has 2 threads, and thus we get 32 threads, but each core isn't really the same.
	// We have to be careful about this with the scheduler and not just see "hey there's 32 threads" and just throw code on whichever CPU we just feel like.
	//At best, only consoles would do it, and seeing as how console systems tend to be low on power consumption, a dual-CPU configuration seems fairly unlikely today.
	//(Especially since games right now don't really utilize that many cores to their full potential... not because the engines are bad, just that no reason to use them.)

	//Of course, don't be bad, actually check if a feature is available before using it.
	*/

	/* First off, ensure we load all the drivers, so we know what's going on. */
	KiPrintLine("Copyright (c) 2018, Brian Schnepp");
	KiPrintLine("Licensed under the Boost Software License.");
	
	SystemInfo.Arch = PROCESSOR_ARCH_X86_64;
	
	/* Do we really need this..? */
	KiPrintLine("If the Boost Software License was not distributed with the OS if it should have, contact your OS vendor with a request for a copy. (this is a test message to demonstrate long strings, and should be removed from release.)");
	KiPrintLine("Loading all drivers...");
	FERALSTATUS KiLoadAllDrivers(VOID);
	KiPrintLine("Preparing execution environment...");
	
#if defined(__x86_64__) || defined(__i386__)
	VgaSetCursorEnabled(1);
	VgaTraceCharacters(1);
	VgaMoveCursor(0, 24);
#endif

	KiPrintFmt("Using math magic, we can equate %u with -1\n", -1L);
	
	/* These are macroed away at release builds.  They're eliminated at build time.*/
	KiDebugPrint("INIT Reached here.");
}

//UINT64 because one day someone is going to do something _crazy_ like have an absurd amount of processors (manycore), but be 32-bit and only 4GB addressable.
//RAM is reasonably cheap (less cheap than before) in 2018, so we don't mind using an unneccessary 7 bytes more than we really need to.
VOID KiStartupProcessor(UINT64 ProcessorNumber)
{
	
}



FERALSTATUS KeBootstrapSystem(VOID)
{
	
}


/* Separate in case needed to implement soft "reboot", ie, just reset OS and RAM contents in memory. */
VOID KiStartupMachineDependent(VOID)
{
#if defined(__x86_64__) || defined(__i386__)

/* Be careful: '__riscv__' is deprecated. */
#elif defined(__riscv)

#elif defined(__aarch64__)

#else
#error Unsupported platform
#endif
}

/* Bring up a system needed for the kernel. */
FERALSTATUS KiStartupSystem(KiSubsystemIdentifier subsystem)
{
	if (subsystem == FERAL_SUBSYSTEM_MEMORY_MANAGEMENT)
	{
	} else {
	}
	return STATUS_SUCCESS;
}


/* temporary, turn into clean later. */
VOID InternalPrintRegister(DWORD reg)
{
	for (int i = 0; i < 4; i++)
	{
		CHAR charToAdd = ((CHAR)(reg >> (i * 8)) & 0xFF);
		VgaPutChar(charToAdd);
	}
}

/* ugly hack, refactor sometime later. */
VOID InternalPrintCpuVendor(DWORD part1, DWORD part2, DWORD part3)
{
	KiPrint(cpu_vendor_msg);
	InternalPrintRegister(part1);
	InternalPrintRegister(part2);
	InternalPrintRegister(part3);
	KiPrintLine("");
}

/*
	 We'll need to implement a proper driver for VGA later. For now, we have something to throw text at and not quickly run out of space.
 */

/*
	For now, kern_init() is multiboot only while I migrate to UEFI. Everything should be EFI because UEFI is ok and not completely horrible. (80s style bios is headache-inducing when the A20 is sometimes on but sometimes not 
	and then sometimes it does odd things with memory or just flat out DOES NOT DO what you expected. ughhh. Compound with speculative execution and out of order execution and it's more effort than it's worth to support a
	dying "standard". That said, UEFI's security is about as solid as swiss cheese, whereas this just doesn't happen with BIOS because the firmware is so small there's not a lot to exploit.)
	AND THE MOST IRRITATING THING OF ALL TIME IS WHEN SOMEONE HAS A BROKEN FIRMWARE IMPLEMENTATION WHERE THE ACPI TABLE IS LYING!!!!!!
	(it's also bad when network cards do this, but we'll just rip the freebsd network stack out so it becomes freebsd's problem)
*/

/* AT LEAST THERE'S NO SECURE BOOT. */

#ifndef FERAL_BUILD_STANDALONE_UEFI_
VOID kern_init(UINT32 MBINFO)
{
	VgaContext graphicsContext = {0};
	UINT8 misc = VgaPrepareEnvironment(&graphicsContext);
	KiBlankVgaScreen(25, 80, VGA_BLACK);
	KiPrintLine("Feral kernel booting...");

	FeralVersionMajor = FERAL_VERSION_MAJOR;
	FeralVersionMinor = FERAL_VERSION_MINOR;
	FeralVersionPatch = FERAL_VERSION_PATCH;

	KiPrintFmt("Starting Feral Kernel Version 00.00.00 %s\n\n", FERAL_VERSION_SHORT);

	//Row 4, index 30, 32, 34, while we're at it, make it green
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0'), 30, 1);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_MAJOR), 31, 1);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0'), 33, 1);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_MINOR), 34, 1);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0'), 36, 1);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_PATCH), 37, 1);
	
	/* First, request the info from the multiboot header. */
	if (MBINFO & 0x07)
	{
		/* Unaligned, go panic: todo, clarify it's a multiboot issue. */
		KiStopError(STATUS_ERROR);
	}
	
	for (multiboot_tag *MultibootInfo = (multiboot_tag*)((UINT64)(MBINFO + 8)); MultibootInfo->type != 0; MultibootInfo = (multiboot_tag*)((UINT8*)(MultibootInfo) + ((MultibootInfo->size + 7) & ~0x07)))
	{
		UINT16 type = MultibootInfo->type;
		if (type == MULTIBOOT_TAG_TYPE_BOOT_LOADER)
		{
			multiboot_tag_string *mb_as_string = (multiboot_tag_string*)(MultibootInfo);
			KiPrint("Detected bootloader: ");
			KiPrintLine(mb_as_string->string);
		} else if (type == MULTIBOOT_TAG_TYPE_BOOT_DEVICE) {
			multiboot_tag_bootdev *mb_as_boot_dev = (multiboot_tag_bootdev*)(MultibootInfo);
			KiPrintFmt("Booted from device %i, slice %i, and partition %i.\nThis will be designated as root (A:/)\n",  mb_as_boot_dev->biosdev, mb_as_boot_dev->slice, mb_as_boot_dev->part);
		}
	}
	
	// We'd like to have some information about the CPU before we boot further.
	// Some things like saying CPU vendor, family, brand name, etc.
	// Eventually, supporting a boot-time flag (and somehow emulating some useful CPU features
	// in-software if not available on the real thing???) would be great.
	// We'll probably use the crypto coprocessor (SHA, etc.) to our advantage with A:/Devices/Hash or something.

	/* This will represent the 4 core registers we need for CPU-specific stuff. */
	DWORD part1 = 0;
	DWORD part2 = 0;
	DWORD part3 = 0;
	DWORD part4 = 0;

	cpuid_vendor_func(&part1, &part2, &part3);
	InternalPrintCpuVendor(part1, part2, part3);	//TODO: cleanup.

	// 'part1' needs to be a reference to the chunk we want (0x8000000[2, 3, 4])
	// This number is then overridden with the appropriate value for the CPUID brand string.
	// As such, we print IMMEDIATELY, then go and replace part1 with the next bit.
	// We may need to load this into a 12-char buffer later.


	for (int i = 0; i < 3; i++)
	{
		part1 = 0x80000002 + i;
		cpuid_brand_name(&part1, &part2, &part3, &part4);
		InternalPrintRegister(part1);
		InternalPrintRegister(part2);
		InternalPrintRegister(part3);
		InternalPrintRegister(part4);
	}

	UINT32 familyStuff = cpuid_family_number();
	UINT32 actualFamily = (familyStuff >> 8) & 15;
	UINT32 extendedModel = (familyStuff >> 16) & (0xF);
	UINT32 baseModel = (familyStuff >> 4) & (0xF);
	UINT32 actualModel = baseModel + (extendedModel << 4);
	
	if (actualFamily == 0x6 || actualFamily == 0xF)
	{
		if (actualFamily == 15)
		{
			actualFamily += (familyStuff >> 20) & 0x0FFFFFFF;
		}
		actualFamily += ((familyStuff >> 16) & 0x4);
	}
	
	
	if ((actualFamily != CPU_x86_64_FAMILY_ZEN))
	{
		/* Now we can not feel bad about using SYSCALL and whatnot in particular ways. */
		KiPrintLine("[WARNING] Unsupported CPU: There may be issues running Feral");
	}

	KiPrintLine("");
	
	// Kernel initialization is done, move on to actual tasks.
	KiSystemStartup();
}
#else
kern_init(UINT32 MBINFO)
{
	/* TODO (uefi standalone) */
}
#endif
