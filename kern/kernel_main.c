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

#if 0
#if defined(__x86_64__) || defined(__i386__)
#include "feral_multiboot2.h"
#endif
#endif	//TODO

#include <feral/boot/kibootstruct.h>
#include <feral/kern/ki.h>

// Private headers use this convention... (TODO: remove the 'inc' part with directly pointing to it)
#include "inc/krnl.h"


#include <arch/processor.h>

static SYSTEM_INFO SystemInfo;

static UINT8 FeralVersionMajor;
static UINT8 FeralVersionMinor;
static UINT8 FeralVersionPatch;

static BOOL ILOVEBEAR18 = 1;	// Maybe you should try bouncing a teddy bear on their monitor? (this is a flag for experimental kernel features)


// FERAL initialization follows a few basic steps:
	/*
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

			It also assigns drive letters (the system default 'A', secondary drives get some way of assigning a letter to them, 
			(usually in alphabetical order (A:/,, B:/, ...,  AB:/, ...,  AZ:/, AAA:/, AAB:/, and so on)
	 */

static CHAR* cpu_vendor_msg = "CPU Vendor: ";


/* 
	Things to do in order:
	Memory management subsystems, RAMFS filesystem
	Simple filesystem (ext2 is enough, 8.3 FAT would be OK to also support (possibly even needed).)
	Create libc, get C++/Rust stuff working on Waypoint. (We would like to make it very easy to create GUI apps for Waypoint, after all.)
	Get a stable userspace up and running. Essentially the syscall table should be similar-ish to Linux, so most assembly programs can be ported over fine.	
	Port LLVM/Clang
	Port LLD, YASM, and the rest of the toolchain. Possibly port a command-line text editor, or just write one from scratch. No GUI, so FreedomEdit isn't possible.

	Work on being self-hosting
	Create a vega10 GPU driver
	Create a desktop environment
	Create an alternative to the LiveCD tools we need currently, create a bootloader that understands (U)EFI. We implement UEFI ourselves, because tangling with BSD license is too much effort.
	Implement Multiboot 2 support
	New filesystem focused on getting as fast read times as possible on files (eliminate file fragmentation by not allowing fragmentation?, yes, more writes, but avoids non-sequential reads.)
	Add some random hypervisor capability just for fun or something.
	Get a working Vulkan driver for the vega10 GPU (Do some magic to port AMDVLK (is this possible?) and/or RADV or something, then just modify them as needed?).
	Port some open source games over, see if we can get it to outperform some other OSes just by running on Feral Waypoint. (specifically, everything we can from the 90s)
	???

	Since we don't want to support anything older than the vega10 GPU, we should just cut out the unnecessary parts of whatever drivers we do adapt.
	(Those are too old, and by the time this becomes usable as a serious OS, those would be *long* obsolete anyway.)
 */

VOID KiSystemStartup(VOID)
{
	//TODO...
	//We need to look for every core on the system (We should expect 8, as we're expecting to run on a ZEN 1700X CPU. I will probably upgrade to something on socket TR4 though.)
	//As such, we need to run a function called HalInitializeProcessor (for the remaining 7 cores... for playing with a 1950X eventually, 15 remaining cores...)
	//I don't have a TR4-compatible CPU (or motherboard) yet, so at this point I'm kind of just hoping $1500 USD just falls out of the sky or something.
	//I do expect the 1950X (as, to my knowledge, it's just a server Zen CPU with two empty areas, and on a different socket.) to act the same as the mainstream models.
	//As such, having one (just to verify it works as intended with even more threads) is unnecessary anyway. (But it would be nice for compile times...)
	//And then call KiInitializeKernel as needed. We'll also have to use SMT when we can.

	//SMP support would also be nice, but I can't really imagine any desktop motherboards supporting a dual-CPU configuration. (though this would be AWESOME! Someone PLEASE make dual-socket TR4.)
	//At best, only consoles would do it, and seeing as how console systems tend to be low on power consumption, a dual-CPU configuration seems fairly unlikely today.
	//(Especially since games right now don't really utilize that many cores to their full potential... not because the engines are bad, just that no reason to use them.)

	//Of course, don't be bad, actually check if a feature is available before using it.


	// First off, ensure we load all the drivers, so we know what's going on.
	FERALSTATUS KiLoadAllDrivers(VOID);
	
	
}

//UINT64 because one day someone is going to do something _crazy_ like have an absurd amount of processors (manycore), but be 32-bit and only 4GB addressable.
//RAM is reasonably cheap (less cheap than before) in 2018, so we don't mind using an unneccessary 7 bytes more than we really need to.
VOID KiStartupProcessor(UINT64 ProcessorNumber)
{
	
}
















// temporary, turn into clean later.
VOID InternalPrintRegister(DWORD reg, DWORD posx, DWORD posy)
{
	for (int i = 0; i < 4; i++)
	{
		CHAR charToAdd = ((CHAR)(reg >> (i * 8)) & 0xFF);
		VgaEntry(VGA_WHITE, VGA_BLACK, charToAdd, posx+i, posy);
	}
}

// ugly hack, refactor sometime later.
VOID InternalPrintCpuVendor(DWORD part1, DWORD part2, DWORD part3)
{
	UINTN strLen = 0;
	KiGetStringLength(cpu_vendor_msg, &strLen);
	InternalPrintRegister(part1, strLen+0, 0);
	InternalPrintRegister(part2, strLen+4, 0);
	InternalPrintRegister(part3, strLen+8, 0);
	VgaStringEntry(VGA_WHITE, VGA_BLACK, cpu_vendor_msg, strLen, 0, 0);
}

//On my laptop, we start in 40x25 for some reason. We REALLY want 80x25 because 40 looks too wide.
//EDIT: grub's doing it. We'll need to use outb and inb and all as needed to manipulate VGA. (ie, a real VGA driver.)
//EDIT: My desktop does it too. So yeah, GRUB puts us in 40x25 for some reason.
// Options are pretty much (in order of difficulty): write a VGA driver, write a vega10 gpu driver, or beg someone with a fab to teach me electronics and design a GPU.
// So, basically for now, we have the first option, and if through some miracle I happen to learn how to mess with FPGAs and get them implemented in real hardware, that too.
// Either way, getting to a vega10 driver is more or less inevitable, it just needs to happen if we want serious support for video games (the primary purpose of Waypoint!!!)

// I actually feel like doing the last one to be honest. Maybe one day I can learn how that stuff works and try to cram low end (520) Fermi-level performance into a 100Mhz FPGA with some crazy RISC SIMD 
// architecture and HBM2 RAM... While Fermi (today) isn't all that great anyway, cramming that *per compute core*, with the FPGA implementing *a single graphics core* (and glue more ala ZEN to it), 
// would be GREAT. (Call it "FX-GSU Graphics Support Unit" or something)
VOID kern_init(void)
{
	UINT8 misc = VgaPrepareEnvironment();
	char* string = "        Feral kernel booting...";	// Why do we have to space 8 times to get this to work? Deleting this line has the next ones go OK. Bug in vga print???
	KiPrintLine(string);

	FeralVersionMajor = FERAL_VERSION_MAJOR;
	FeralVersionMinor = FERAL_VERSION_MINOR;
	FeralVersionPatch = FERAL_VERSION_PATCH;

	// temporary, hackish, just for now until I implement a proper printk().
	char* verString = "Starting Feral Kernel Version 0.0.0";
	KiPrintLine(verString);

	//Row 4, index 30, 32, 34, while we're at it, make it green
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_MAJOR), 30, 4);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_MINOR), 32, 4);
	VgaEntry(VGA_GREEN, VGA_BLACK, ('0' + FERAL_VERSION_PATCH), 34, 4);

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
		InternalPrintRegister(part1, 0  + (16 * i), 2);
		InternalPrintRegister(part2, 4  + (16 * i), 2);
		InternalPrintRegister(part3, 8  + (16 * i), 2);
		InternalPrintRegister(part4, 12 + (16 * i), 2);
	}

	VgaSetCursorEnabled(1);
	/* Below are just tests to make sure the extremely primitive VGA driver works as intended. */
	char* Stringy = "Hello, world!!!111";
	for (int k = 0; k < 3; k++)
	{
		KiPrintLine(Stringy);
	}
	for (int k = 0; k < 3; k++)
	{
		KiPrintLine("");
	}
	VgaTraceCharacters(1);
	KiPrintLine("HELLO!");
	KiPrintLine("AAA");
	KiPrintLine("BBB");
	KiPrintLine("AL");
	KiPrintLine("HELLO!");
	KiPrintLine("AAA");
	KiPrintLine("BBB");
	VgaMoveCursor(4, 6);	//This doesn't seem to quite work as expected. Is my assembler code wrong?

	InternalPrintRegister((misc | 0x65656565), 0, 10);

	// Kernel initialization is done, move on to actual tasks.
	FERALSTATUS KiLoadAllDrivers(VOID);
	KiSystemStartup();
}
