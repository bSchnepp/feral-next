/*
Copyright (c) 2020 Brian Schnepp

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
#include <stdint.h>

#if defined(__x86_64__)
#include <arch/x86_64/cpuio.h>
#include <arch/x86_64/cpufuncs.h>
#endif

/* Borrow a bunch from the Feralboot. */
#ifndef _NO_UEFI_STUB_
#include <libreefi/efi.h>
#endif

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


/* hack: include the serial driver in a brute forcey way. */
#include <drivers/devices/serial/serial.h>


/* hack for now */
static UINT64 FreeMemCount;

/* Support up to 8 regions. Hack for now until we get a real malloc. */
static UINT_PTR FreeMemLocs[16];

/*
	 We'll need to implement a proper driver for VGA later. 
	 For now, we have something to throw text at and not quickly run out 
	 of space.
 */

#ifndef FERAL_BUILD_STANDALONE_UEFI_
#include "multiboot/multiboot2.h"
#include <drivers/proc/elf/elf.h>
#include <arch/x86_64/vga/vga.h>

static CHAR *cpu_vendor_msg = "CPU Vendor: ";


/* temporary, turn into clean later. */
VOID InternalPrintRegister(UINT32 reg)
{
	for (int i = 0; i < 4; i++)
	{
		CHAR charToAdd = ((CHAR)(reg >> (i * 8)) & 0xFF);
		VgaAutoEntry(VGA_GREEN, VGA_BLACK, charToAdd);
	}
}

/* ugly hack, refactor sometime later. */
VOID InternalPrintCpuVendor(UINT32 part1, UINT32 part2, UINT32 part3)
{
	VgaAutoPrint(VGA_GREEN, VGA_BLACK, cpu_vendor_msg);
	InternalPrintRegister(part1);
	InternalPrintRegister(part2);
	InternalPrintRegister(part3);
	VgaAutoPrintln(VGA_WHITE, VGA_BLACK, "");
}


static VgaContext graphicsContext = {0};
static UINT16 OtherBuffer[80 * 25];
static KrnlFirmwareFunctions FirmwareFuncs = {0};
static KrnlCharMap CharMap = {0};
static KrnlEnvironmentBlock EnvBlock = {0};

STRING GetBiosFirmwareClaim();
VOID InternalVgaPrintln(STRING Str, UINT64 Len);

STRING GetBiosFirmwareClaim()
{
	return "PC Compatible BIOS (Multiboot 2)";
}


VOID InternalVgaPrintln(STRING Str, UINT64 Len)
{
	VgaPrintln(VGA_WHITE, VGA_BLACK, Str, Len);
}

VOID kern_init(UINT32 MBINFO)
{
	UINT8 misc = VgaPrepareEnvironment(&graphicsContext);
	graphicsContext.SwappedBuffer = OtherBuffer;
	KiBlankVgaScreen(25, 80, VGA_BLACK);
	VgaAutoPrintln(VGA_WHITE, VGA_BLACK,
		"Starting initial kernel setup...");
	/* First, request the info from the multiboot header. */
	if (MBINFO & 0x07)
	{
		/* Unaligned, go panic: todo, clarify it's a multiboot issue. */
		KiStopError(STATUS_ERROR);
	}
	/* Some necessary pointer magic is needed to make this work.
	 * Multiboot is a little difficult to parse in a clear and easy way,
	 * so this is needed to make it work well.
	 *
	 * First, force cast the argument to multiboot info. Then,
	 * some info needs to get queried from it, like each of it's tags.
	 * To check the tags, the real pointer magic begins by taking
	 * an offset by adding the size of the items which are in the way
	 * to the base pointer, force casing this, and looping until the
	 * end tag is reached.
	 *
	 * Each of these tags is then processed based on the ID of it.
	 */
	for (multiboot_tag
			*MultibootInfo
		= (multiboot_tag *)((UINT_PTR)(MBINFO + 8));
		MultibootInfo->type != MULTIBOOT_TAG_TYPE_END;
		MultibootInfo = (multiboot_tag *)((UINT8 *)(MultibootInfo)
						  + ((MultibootInfo->size + 7) & ~0x07)))
	{
		UINT16 Type = MultibootInfo->type;
		if (Type == MULTIBOOT_TAG_TYPE_CMD_LINE)
		{
			multiboot_tag_string *mb_as_string
				= (multiboot_tag_string *)(MultibootInfo);
			STRING str = mb_as_string->string;
			UINT64 len = 0;
			if (KiGetStringLength(str, &len) == STATUS_SUCCESS)
			{
				if (len != 0)
				{
					VgaAutoPrint(VGA_WHITE, VGA_BLACK,
						"Got command line: ");
					VgaAutoPrintln(VGA_RED, VGA_BLACK,
						mb_as_string->string);
				}
			}
		}
		else if (Type == MULTIBOOT_TAG_TYPE_BOOT_LOADER)
		{
			multiboot_tag_string *mb_as_string
				= (multiboot_tag_string *)(MultibootInfo);
			VgaAutoPrint(VGA_WHITE, VGA_BLACK, "Detected bootloader: ");
			VgaAutoPrintln(VGA_LIGHT_BROWN, VGA_BLACK, mb_as_string->string);
		}
		else if (Type == MULTIBOOT_TAG_TYPE_BOOT_DEVICE)
		{
			multiboot_tag_bootdev *mb_as_boot_dev
				= (multiboot_tag_bootdev *)(MultibootInfo);
			/* Care about this stuff later... */
		}
		else if (Type == MULTIBOOT_TAG_TYPE_MEM_MAP)
		{
			/* Memory map detected... MB2's kludgy mess here makes this a little painful, but we'll go through this step-by-step.*/
			multiboot_tag_mmap *mb_as_mmap_items = (multiboot_tag_mmap *)(MultibootInfo);
			multiboot_mmap_entry currentEntry = {0};

			UINT64 index = 0;
			UINT64 maxIters = (mb_as_mmap_items->size) / mb_as_mmap_items->entry_size;

			/* Issue: We add region for every possible area. They're not all free, so we have bigger buffer than needed. */
			FreeMemCount = maxIters;
			UINT64 FreeAreasWritten = 0;


			for (currentEntry = mb_as_mmap_items->entries[0]; index < maxIters; currentEntry = mb_as_mmap_items->entries[++index])
			{
				/* Check the Type first. */
				if (currentEntry.type == E820_MEMORY_TYPE_FREE)
				{
					/* Write 1: Start pointer */
					FreeMemLocs[FreeAreasWritten] = (UINT_PTR)currentEntry.addr;
					/* Write 2: End pointer */
					FreeMemLocs[FreeAreasWritten + 1] = (UINT_PTR)currentEntry.addr + (UINT_PTR)currentEntry.len;
					FreeAreasWritten += 2;
				}
				else if (currentEntry.type == E820_MEMORY_TYPE_ACPI)
				{
					/* 16 chars will fit the whole address*/
					CHAR BufBeginAddr[17];
					CHAR BufEndAddr[17];
					internalItoaBaseChange(currentEntry.addr, BufBeginAddr, 16);
					internalItoaBaseChange(currentEntry.addr + currentEntry.len, BufBeginAddr, 16);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, "ACPI memory at: 0x");
					VgaAutoPrint(VGA_GREEN, VGA_BLACK, BufBeginAddr);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, ", up to 0x");
					VgaAutoPrintln(VGA_RED, VGA_BLACK, BufEndAddr);
				}
				else if (currentEntry.type == E820_MEMORY_TYPE_NVS)
				{
					/* 16 chars will fit the whole address*/
					CHAR BufBeginAddr[17];
					CHAR BufEndAddr[17];
					internalItoaBaseChange(currentEntry.addr, BufBeginAddr, 16);
					internalItoaBaseChange(currentEntry.addr + currentEntry.len, BufBeginAddr, 16);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, "Reserved hardware memory at: 0x");
					VgaAutoPrint(VGA_GREEN, VGA_BLACK, BufBeginAddr);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, ", up to 0x");
					VgaAutoPrintln(VGA_RED, VGA_BLACK, BufEndAddr);
				}
				else if (currentEntry.type == E820_MEMORY_TYPE_BADMEM)
				{
					/* 16 chars will fit the whole address*/
					CHAR BufBeginAddr[17];
					CHAR BufEndAddr[17];
					internalItoaBaseChange(currentEntry.addr, BufBeginAddr, 16);
					internalItoaBaseChange(currentEntry.addr + currentEntry.len, BufBeginAddr, 16);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, "BAD MEMORY at: 0x");
					VgaAutoPrint(VGA_GREEN, VGA_BLACK, BufBeginAddr);
					VgaAutoPrint(VGA_WHITE, VGA_BLACK, ", up to 0x");
					VgaAutoPrintln(VGA_RED, VGA_BLACK, BufEndAddr);
				}

				/* E820_MEMORY_TYPE_RESERVED, E820_MEMORY_TYPE_DISABLED, E820_MEMORY_TYPE_INV ignored. */
			}
			FreeMemCount = FreeAreasWritten >> 1;
		}
		else if (Type == MULTIBOOT_TAG_TYPE_ELF_SECTIONS)
		{
			/* For now, we'll just use the ELF sections tag. */
			multiboot_tag_elf_sections *mb_as_elf = (multiboot_tag_elf_sections *)(MultibootInfo);

			/* 20 is here because multiboot said so. Don't know why, but that's the size, so go with it. */
			UINT64 maxIters = (mb_as_elf->size - 20) / (mb_as_elf->entsize);
			UINT64 index = 0;

			/* Don't care here... */

			for (UINT64 i = 0; i < maxIters; i++)
			{
				ElfSectionHeader64 *currentEntry = (&mb_as_elf->sections[i * mb_as_elf->entsize]);
				/* todo... */
			}
		}
	}

	// We'd like to have some information about the CPU before we boot further.
	// Some things like saying CPU vendor, family, brand name, etc.
	// Eventually, supporting a boot-time flag (and somehow emulating some useful CPU features
	// in-software if not available on the real thing???) would be great.
	// We'll probably use the crypto coprocessor (SHA, etc.) to our advantage with A:/Devices/Hash or something.

	/* This will represent the 4 core registers we need for CPU-specific stuff. */
	UINT32 part1 = 0;
	UINT32 part2 = 0;
	UINT32 part3 = 0;
	UINT32 part4 = 0;

	cpuid_vendor_func(&part1, &part2, &part3);
	InternalPrintCpuVendor(part1, part2, part3);//TODO: cleanup.

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
	VgaAutoPrintln(VGA_WHITE, VGA_BLACK, ""); /* Flush to newline. */

	UINT32 familyStuff = cpuid_family_number();
	UINT32 actualFamily = (familyStuff >> 8) & 15;
	UINT32 extendedModel = (familyStuff >> 16) & (0xF);
	UINT32 baseModel = (familyStuff >> 4) & (0xF);
	UINT32 actualModel = baseModel + (extendedModel << 4);

	if (actualFamily == 0x6 || actualFamily == 0xF)
	{
		/* Family 15 wants us to report the family number as such. */
		if (actualFamily == 15)
		{
			actualFamily += (familyStuff >> 20) & 0x0FFFFFFF;
		}
		/* In both cases, we also need to append some more info from cpuid. */
		actualFamily += ((familyStuff >> 16) & 0x4);
	}


	if ((actualFamily != CPU_x86_64_FAMILY_ZEN))
	{
		/* 
			Trim down the error messages. TODO: Inspect model 
			number. If the model number *is* Zen, but not *first 
			generation Zen*, then also complain about unsupported 
			CPU.
			
			We identify this based on the presence of "1", as in
			1950X, 1700X, 1800X, etc. We must *also* check for "2"
			and "3", since 2000-series are all either Zen+ 
			(supported for sure) or zen (also supported for sure.) 
			We have these tight checks because inevitably I will 
			make a mistake somewhere and use FMA3/RDRAND/etc in a 
			Zen-specific way at some point probably, and won't get 
			around to fixing it for a while. Also because the way
			the scheduler is going to be built is *specifically* for
			Zen's chiplet, really-big-cache design. It'll *probably*
			be fine for Zen 2, but I dont have the hardware so I 
			can't prove it, even anecdotally. The scheduler is to be
			built with the latency between chiplets in mind, and 
			trying our hardest to avoid shuffling between cores, and
			especially between CCXs.
			
			Do note that some APUs ("3000-series") are actually 
			Zen+, not Zen 2. These are identified with the suffix 
			"GE" and "G".
			
			Then we have to deal with the embedded CPUs. 
			(V, R-series). Both of these are Zen 1.
		*/
		VgaAutoPrintln(VGA_RED, VGA_BLACK, "Unsupported CPU");
	}
	if (TRUE)
	{
		/* 
			Put this here until we get B350, X370, etc. drivers.
			Feral (for now) doesn't support _any_ platform 
			controller hub, so this message is pretty much 
			meaningless.
			
			We need to actually write code to identify between
			X370, X470, X570, B350, B450, B550, B320, B420, and 
			B520. (oh and panther point and wellsburg or whatever 
			for the blue team if we care.)
			
			We'll need these for doing chipset-specific stuff
			one day, like telling the x570 to do RAID, or
			nicely asking about USB devices connected to it and
			not the CPU directly. (over PCI, of course.)
		 */
		VgaAutoPrintln(VGA_RED, VGA_BLACK, "Unsupported PCH");
	}

	VgaSetCursorEnabled(TRUE);
	VgaTraceCharacters(TRUE);
	VgaMoveCursor(0, 24);

	FirmwareFuncs.PutChar = VgaPutChar;
	FirmwareFuncs.Println = InternalVgaPrintln;
	FirmwareFuncs.GetFirmwareName = GetBiosFirmwareClaim;
	EnvBlock.FunctionTable = &FirmwareFuncs;
	EnvBlock.CharMap = &CharMap;
	/* Kernel initialization is done, move on to actual tasks. */
	KiSystemStartup(&EnvBlock);
}

/* FIXME: Genericize and move back out. */

/* Bring up a system needed for the kernel. */
FERALSTATUS KiStartupSystem(KiSubsystemIdentifier subsystem)
{
	if (subsystem == FERAL_SUBSYSTEM_MEMORY_MANAGEMENT)
	{
		MmPhysicalAllocationInfo allocInfo;
		allocInfo.sType = MM_STRUCTURE_TYPE_PHYSICAL_ALLOCATION_INFO;
		allocInfo.pNext = (void *)(0);
		allocInfo.FrameSize = 4096;

		allocInfo.FreeAreaRangeCount = FreeMemCount;
		MmFreeAreaRange ranges[FreeMemCount];
		for (UINT64 i = 0; i < FreeMemCount; ++i)
		{
			ranges[i].sType = MM_STRUCTURE_TYPE_FREE_AREA_RANGE;
			ranges[i].pNext = (void *)(0);
			/* We need in groups of 2. i gives us the group of 2
			   at present.
			 */
			ranges[i].Start = FreeMemLocs[i * 2];
			ranges[i].End = FreeMemLocs[(i * 2) + 1];
			ranges[i].Size = ranges[i].End - ranges[i].Start;
		}
		allocInfo.Ranges = ranges;
		MmCreateInfo info;

		info.sType = MM_STRUCTURE_TYPE_MANAGEMENT_CREATE_INFO;
		info.pNext = NULLPTR;
		info.pPhysicalAllocationInfo = &allocInfo;
		return KiInitializeMemMgr(&info);
	}
	else if (subsystem == FERAL_SUBSYSTEM_ARCH_SPECIFIC)
	{
		KiStartupMachineDependent();
		/* TODO: Enable serial driver
		 * iff use-serial=true, instead of
		 * whenever there is the
		 * command line...
		 */
		VOID *Databack = NULLPTR;
		InitSerialDevice(Databack);
		for (;;) {}
	}
	else
	{
		/*  Placeholder for more stuff later on. (disks, network...) */
	}
	return STATUS_SUCCESS;
}

#endif
