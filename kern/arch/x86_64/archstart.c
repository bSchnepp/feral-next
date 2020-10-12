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


#include <arch/x86_64/cpuio.h>
#include <arch/x86_64/cpuinfo.h>
#include <arch/x86_64/cpufuncs.h>
#include <arch/x86_64/idt/idt.h>

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>
#include <feral/kern/frlos.h>
#include <mm/mm.h>

#include <feral/kern/krnlfuncs.h>
#include <feral/kern/krnlmacro.h>
#include <feral/kern/krnlbase.h>

#include <krnl.h>
#include <kern_ver.h>

static GDTPointer GlobalGDT;
static GDTEntry GDTEntries[5];

static IDTDescriptor IDT[256];
static IDTPointer IDTPTR;

VOID x86InitializeGDT();

VOID KiStartupMachineDependent(VOID)
{
	/* Possibly for things like PCI scanning or something. NYI. */
}

VOID x86_install_gdt(GDTPointer *Pointer);



VOID x86InitializeGDT()
{
	/* setup gdt and idt... */
	UINT16 Limit = 5;

	/* Set up null descriptor */
	GDTEntries[0].Base = 0;
	GDTEntries[0].High = 0;
	GDTEntries[0].Limit = 0;

	/* Set up kernel code */
	GDTEntries[1].Base = 0;
	GDTEntries[1].High = 0;
	GDTEntries[1].Limit = 0;
	GDTEntries[1].AsBits.SixtyFourMode = 1;
	GDTEntries[1].AsBits.Present = 1;
	GDTEntries[1].AsBits.System = 1;
	GDTEntries[1].AsBits.Executable = 1;
	GDTEntries[1].AsBits.ReadWritable = 1;
	GDTEntries[1].AsBits.Granularity = 1;

	/* Set up kernel data. */
	GDTEntries[2].Base = 0;
	GDTEntries[2].High = 0;
	GDTEntries[2].Limit = 0;
	GDTEntries[2].AsBits.SixtyFourMode = 1;
	GDTEntries[2].AsBits.Present = 1;
	GDTEntries[2].AsBits.System = 1;
	GDTEntries[2].AsBits.ReadWritable = 1;

	/* Set up user code */
	GDTEntries[3].Base = 0;
	GDTEntries[3].High = 0;
	GDTEntries[3].Limit = 0;
	GDTEntries[3].AsBits.SixtyFourMode = 1;
	GDTEntries[3].AsBits.Present = 1;
	GDTEntries[3].AsBits.System = 1;
	GDTEntries[3].AsBits.Executable = 1;
	GDTEntries[3].AsBits.ReadWritable = 1;
	GDTEntries[3].AsBits.Granularity = 1;
	GDTEntries[3].AsBits.PrivLevel = 3;

	/* Set up user data. */
	GDTEntries[4].Base = 0;
	GDTEntries[4].High = 0;
	GDTEntries[4].Limit = 0;
	GDTEntries[4].AsBits.SixtyFourMode = 1;
	GDTEntries[4].AsBits.Present = 1;
	GDTEntries[4].AsBits.System = 1;
	GDTEntries[4].AsBits.ReadWritable = 1;
	GDTEntries[4].AsBits.PrivLevel = 3;

	GlobalGDT.Limit = (sizeof(GDTEntry) * 5) - 1;
	GlobalGDT.Base = (UINT64)(GDTEntries);
	x86_install_gdt(&GlobalGDT);
}

void x86InitializeIDT()
{
	/* Make the PIT happy. */
	INT32 Divisor = 11931840;
	x86outb(0x43, 0x36); /* Tell the PIT to accept it. */
	x86outb(0x40, (Divisor >> 0) & 0xFF);
	x86outb(0x40, (Divisor >> 8) & 0xFF);

	/* Initialize the PICs. 0x10 for INIT, and 0x01 for disabling stuff. */
	KiSetMemoryBytes(IDT, 0, (sizeof(IDTDescriptor)) * 256);
	x86outb(X86_PIC_1_COMMAND, (0x10 | 0x01));
	x86outb(X86_PIC_2_COMMAND, (0x10 | 0x01));
	x86_io_stall();

	/* Do the remap! */
	x86outb(X86_PIC_1_DATA, 0x20); /* First 7 interrupts */
	x86outb(X86_PIC_2_DATA, 0x28); /* Last 8 interrupts */
	x86_io_stall();

	/* Handle the cascades. */
	x86outb(X86_PIC_1_DATA, 0x04); /* First 7 interrupts */
	x86outb(X86_PIC_2_DATA, 0x02); /* Last 8 interrupts */
	x86_io_stall();

	/* Environment info... */
	x86outb(X86_PIC_1_DATA, 0x01); /* First 7 interrupts */
	x86outb(X86_PIC_2_DATA, 0x01); /* Last 8 interrupts */
	x86_io_stall();

	/* Leave this alone for now... */
	x86outb(X86_PIC_1_DATA, 0x01); /* First 7 interrupts */
	x86outb(X86_PIC_2_DATA, 0x01); /* Last 8 interrupts */
	x86_io_stall();

	x86outb(X86_PIC_1_DATA, 0xFC); /* Only allow a few IRQs. For now. */
	x86outb(X86_PIC_2_DATA, 0x80); /* Allow all the PIC 2 IRQs..? */

	IDTPTR.Limit = ((sizeof(IDTDescriptor)) * 256) - 1;
	UINT_PTR Location = (&IDT);
	IDTPTR.Location = Location;
	x86SetupIDTEntries();

	x86_install_idt(&IDTPTR);
	KiRestoreInterrupts(TRUE);

	KiPrintFmt("IDT Ready to work...\n");
}

volatile void x86IDTSetGate(
	UINT8 Number, UINT_PTR Base, UINT16 Selector, UINT8 Flags)
{
	/* 0 - 255 happens to be valid, so no need for checking. */
	IDTDescriptor Descriptor = {0};

	Descriptor.Offset = (UINT16)(Base & 0xFFFF);
	Descriptor.Offset2 = (UINT16)((Base >> 16) & 0xFFFF);
	/*
		Reserved should stay reserved. (on 32-bit x86,
		these are different fields, but the function
		is the same: don't do anything with it.
	*/
	Descriptor.RESERVED = 0;

	/* And the important bits. */
	Descriptor.Selector = Selector;
	Descriptor.TypeAttr = Flags;

/* Handful of embedded x86s out there. May want to support one day. */
#if defined(__x86_64__)
	Descriptor.Offset3 = (UINT32)((Base >> 32) & 0xFFFFFFFF);
	/* No TSS, so set to zero. */
	Descriptor.IST = 0;
#endif
	IDT[Number] = Descriptor;
}

VOID KiStartupProcessorMachineDependent(UINT32 Core)
{
	KiRestoreInterrupts(FALSE);
	if (Core == 0)
	{
		/* If we're here, that means the GDT is still in low memory.
		 * We don't want that, so let's make a new one.
		 */
		x86InitializeGDT();

		/* Unmap low memory, since GDT was last leftover. */
		x86UnmapAddress(get_initial_p4_table(), 0);

		/* Now set up global IDT. */
		x86InitializeIDT();
	}
	KiRestoreInterrupts(TRUE);
}

COMPILER_ASSERT(sizeof(GDTEntry) == sizeof(UINT_PTR));
COMPILER_ASSERT(sizeof(GDTPointer) == sizeof(IDTPointer));
COMPILER_ASSERT(sizeof(PageMapEntry) == sizeof(UINT_PTR));

COMPILER_ASSERT(sizeof(GDTPointer) > sizeof(UINT64));
COMPILER_ASSERT(sizeof(IDTPointer) > sizeof(UINT64));