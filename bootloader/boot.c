/*
Copyright (c) 2020, Brian Schnepp

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

/* NOTE: The terms "BIOS", "UEFI" and "firmware" are used INTERCHANGABLY.
 * For the PC-compatible BIOS, "PC-compatible BIOS" is used instead to
 * differentiate between UEFI firmware and PC-compatible BIOS.
 *
 * (For other examples of this convention, see the use of the term "SSL" 
 * to refer to TLS.)
 */

#include <libreefi/efi.h>
#include <feral/stdtypes.h>
#include <feral/feralstatus.h>
#include <feral/kern/krnlbase.h>
#include <feral/kern/krnlentry.h>

#ifndef _FRLBOOT_NO_SUPPORT_ELF64_
#include <drivers/proc/elf/elf.h>

#if defined(__x86_64__)
#define CUR_ARCH_ELF MACHINE_ID_X86_64
#endif
#else
#error "EFI loader not supported for this architecture"
#endif


/* How much does the kernel */
#define MAX_INIT_PAGED (2 * 1024 * 1024)

#define EFI_PAGE_SIZE (4096)
#define EFI_PAGE_SHIFT (12)
#define EFI_PAGE_MAP (~(EFI_PAGE_SIZE - 1))
#define FERAL_VIRT_OFFSET (0xFFFFFFFFC0000000)

/* We need these GUIDs to load the protocols we want. */
static EFI_GUID GuidEfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;

static EFI_GUID GuidEfiSimpleFSProtocol = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

static EFI_GUID GuidEfiFileInfoGuid = EFI_FILE_INFO_ID;

static EFI_GUID GuidEfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;



/* The EFI table stuff given to us by the firmware. */
static EFI_HANDLE ImageHandle;
static EFI_SYSTEM_TABLE *SystemTable;

/* Firmware function to open stuff. */
static EFI_OPEN_PROTOCOL OpenProtocol;

static CHAR16 *KernelLoc = L"\\EFI\\Feral\\FERALKER.NEL";

VOID WriteMessage(CHAR16 *String)
{
	SystemTable->ConOut->OutputString(SystemTable->ConOut, String);
}

UINT64 BytesToEfiPages(UINT64 Bytes)
{
	/* Lazy implementation for debugging... */
	UINT64 RetVal = Bytes >> EFI_PAGE_SHIFT;
	if (RetVal % EFI_PAGE_SIZE || RetVal == 0)
	{
		++RetVal;
	}
	return RetVal;
}

/* Borrowed from krnlfuncs... */
VOID InternalItoaBaseChange(UINT64 Val, CHAR16 *Buf, UINT8 Radix)
{
	UINT64 Len = 0;
	UINT64 ValCopy = 0;
	UINT64 ReverseIndex = 0;

	/* No point in continuing if it's zero. Just give '0' back. */
	if (Val == 0)
	{
		*Buf = L'0';
		*(Buf + 1) = L'\0';
		return;
	}
	/* Weird behavior if you exceed 10 + 26 + 26 as radix. */
	for (ValCopy = Val; ValCopy != 0; ValCopy /= Radix)
	{
		/* We need the remainder to see how to encode. */
		CHAR16 Rem = ValCopy % Radix;
		if (Rem <= 9 && Rem >= 0)
		{
			/* It's already a number. Just encode in UTF16. */
			Buf[Len++] = Rem + L'0';
		}
		else if (Rem < 35)
		{
			/* Encode the number as a lowercase letter. */
			Buf[Len++] = (Rem - 10) + L'a';
		}
		else
		{
			/* Encode as uppercase. */
			Buf[Len++] = (Rem - 36) + L'A';
		}
	}

	/* It's written BACKWARDS. So flip the order of the string. */
	for (ReverseIndex = 0; ReverseIndex < Len / 2; ++ReverseIndex)
	{
		CHAR16 Tmp = Buf[ReverseIndex];
		Buf[ReverseIndex] = Buf[Len - ReverseIndex - 1];
		Buf[Len - ReverseIndex - 1] = Tmp;
	}

	/* Terminate the string. */
	Buf[Len] = '\0';
}

BOOL GetEfiMemoryMapToFreeOrNot(EFI_MEMORY_DESCRIPTOR *Place)
{
	switch (Place->Type)
	{
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
		{
			return TRUE;
		}

		default:
		{
			return FALSE;
		}
	}
}

BOOL EfiMalloc(UINT64 Amt, VOID **Ptr)
{
	EFI_STATUS Status = SystemTable->BootServices->AllocatePool(
		EfiRuntimeServicesData,
		Amt,
		Ptr);
	return Status == EFI_SUCCESS;
}

BOOL EfiAllocAddr(UINT64 Amt, VOID *Ptr, BOOL Code)
{
	EFI_MEMORY_TYPE MemType = EfiRuntimeServicesData;
	if (Code)
	{
		MemType = EfiRuntimeServicesCode;
	}

	EFI_STATUS Status = SystemTable->BootServices->AllocatePages(
		AllocateAddress,
		MemType,
		BytesToEfiPages(Amt),
		Ptr);

	return Status == EFI_SUCCESS;
}

/**
 * Allocates some memory which does not exceed a certain area.
 * @author Brian Schnepp
 * 
 * @param Amt The amount, in bytes, to have allocated.
 * @param Ptr The resulting area where memory was allocated
 * @param MaxAddress The maximum address of the last byte of the allocation.
 * @param Code True if this should be executable, false otherwise.
 */
BOOL EfiAllocMaxAddr(UINT64 Amt, VOID **Ptr, UINTN MaxAddress, BOOL Code)
{
	EFI_MEMORY_TYPE MemType = EfiRuntimeServicesData;
	if (Code)
	{
		MemType = EfiRuntimeServicesCode;
	}

	UINTN Addr = (MaxAddress - Amt);
	EFI_STATUS Status = SystemTable->BootServices->AllocatePages(
		AllocateMaxAddress,
		MemType,
		BytesToEfiPages(Amt),
		&(Addr));

	*Ptr = Addr;
	return Status == EFI_SUCCESS;
}

EFI_STATUS ValidateElfHeader(ElfHeader64 *Header)
{
	/* Check the header... */
	CHAR Match[4] = {0x7F, 'E', 'L', 'F'};
	for (UINT32 Index = 0; Index < sizeof(Match); ++Index)
	{
		if (Header->magic[Index] != Match[Index])
		{
			return EFI_LOAD_ERROR;
		}
	}

	if (Header->e_machine != MACHINE_ID_NONE
		&& Header->e_machine != CUR_ARCH_ELF)
	{
		return EFI_LOAD_ERROR;
	}

	if (Header->e_phnum == 0)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Not an executable: missing program headers... \r\n");
		return EFI_LOAD_ERROR;
	}
	return STATUS_SUCCESS;
}

EFI_STATUS EFIAPI ElfLoadFile(IN EFI_FILE_PROTOCOL *File, OUT VOID **Entry)
{
	ElfHeader64 InitialHeader;
	ElfProgramHeader64 ProgramHeader;

	EFI_PHYSICAL_ADDRESS PAddress = NULLPTR;

	UINT64 HdrSize = sizeof(ElfHeader64);
	UINT64 PHdrSize = sizeof(ElfProgramHeader64);

	/* Extra space, just to be safe. */
	CHAR16 ItoaBuf[32];

	EFI_STATUS Status = File->Read(File, &HdrSize, &InitialHeader);
	if (Status != EFI_SUCCESS)
	{
		return Status;
	}

	Status = ValidateElfHeader(&InitialHeader);
	if (Status != EFI_SUCCESS)
	{
		return Status;
	}

	UINT64 BaseAddr = 0;

	/* Change the position to the area currently cared about. */
	for (UINT32 Index = 0; Index < InitialHeader.e_phoff; ++Index)
	{
		/* Read the program data in... */
		File->SetPosition(File, InitialHeader.e_phoff
						+ InitialHeader.e_phentsize * Index);

		/* Read it in... */
		File->Read(File, &PHdrSize, &ProgramHeader);

		if (ProgramHeader.p_type == PT_LOAD)
		{
			EFI_PHYSICAL_ADDRESS PAddr
				= ProgramHeader.p_vaddr;
			if (PAddr > FERAL_VIRT_OFFSET)
			{
				PAddr -= FERAL_VIRT_OFFSET;
			}

			/* 0x01 is the executable flag.
			 * Remember to replicate that in the ELF header!
			 *  (TODO on that)
			 */
			Status = EfiAllocAddr(ProgramHeader.p_memsz,
				PAddr,
				ProgramHeader.p_flags & 0x01);

			if (Status != EFI_SUCCESS)
			{
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					L"EfiAllocAddr failed...\r\n");
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					EfiErrorToString(Status));
				SystemTable->BootServices->Stall(12500000);
				return Status;
			}

			/* Go ahead and bulldoze whatever memory was there,
			 * and load the kernel there.
			 */
			Status = File->SetPosition(File, ProgramHeader.p_offset);
			if (Status != EFI_SUCCESS)
			{
				return Status;
			}

			/* Load the ELF contents to that location. */
			UINT8 *EfiMem = NULLPTR;
			UINTN FileSz = ProgramHeader.p_filesz;
			Status = File->Read(
				File,
				&FileSz,
				EfiMem);

			if (Status != EFI_SUCCESS)
			{
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					EfiErrorToString(Status));
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					L" : Failed to load program section.\r\n");
				SystemTable->BootServices->Stall(10000);
			}

			for (UINT64 Index = 0; Index < ProgramHeader.p_filesz;
				++Index)
			{
				UINT8 *Out = (UINT8 *)(PAddr);
				Out[Index] = EfiMem[Index];
			}
		}
	}

	/* Entry is already in vaddr. Ensure we jump right. */
	*Entry = (InitialHeader.e_entry);
	if (*Entry > FERAL_VIRT_OFFSET)
	{
		*Entry -= FERAL_VIRT_OFFSET;
	}

	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Kernel (virt) entry is at: 0x");
	InternalItoaBaseChange(InitialHeader.e_entry,
		ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");
	return EFI_SUCCESS;
}

EFI_STATUS SetupEfiFramebufferData(EFI_HANDLE *VideoBuffers,
	UINT_PTR *DisplayBuffers,
	UINT64 NumDisplays)
{
	/* Pointer to the GOP, temporarilly at least. */
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsProtocol = NULLPTR;
	CHAR16 ItoaBuf[32];

	for (UINT64 Iterator = 0; Iterator < NumDisplays; Iterator++)
	{
		/* Get the current GOP. */
		EFI_STATUS Result = OpenProtocol(VideoBuffers[Iterator],
			&GuidEfiGraphicsOutputProtocol,
			(VOID **)&GraphicsProtocol,
			ImageHandle, NULL,
			EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

		if (Result != EFI_SUCCESS)
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				L"Failed to query display...\r\n");
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				EfiErrorToString(Result));
			SystemTable->BootServices->Stall(12500000);
			return Result;
		}

		Result = GraphicsProtocol->SetMode(GraphicsProtocol,
			PixelRedGreenBlueReserved8BitPerColor);
		if (Result != EFI_SUCCESS)
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				L"Display does not use 32-bit RGB mode...\r\n");
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				EfiErrorToString(Result));
			SystemTable->BootServices->Stall(12500000);
			return Result;
		}

		/* Update the current width and height, then display. */
		UINT64 CurrentWidth
			= GraphicsProtocol->Mode->Info->HorizontalResolution;
		UINT64 CurrentHeight
			= GraphicsProtocol->Mode->Info->VerticalResolution;

		/* HACK: first and second are sizes.*/
		DisplayBuffers[(3 * Iterator) + 0] = CurrentWidth;
		DisplayBuffers[(3 * Iterator) + 1] = CurrentHeight;

		DisplayBuffers[(3 * Iterator) + 2]
			= GraphicsProtocol->Mode->FrameBufferBase;

		/* And tell the user that we found it! */
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Got display of width: ");
		InternalItoaBaseChange(CurrentWidth, ItoaBuf, 10);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			ItoaBuf);

		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L" and height of ");
		InternalItoaBaseChange(CurrentHeight, ItoaBuf, 10);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			ItoaBuf);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"\r\n");

		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Which has it's framebuffer at: 0x");
		InternalItoaBaseChange(GraphicsProtocol->Mode->FrameBufferBase,
			ItoaBuf, 16);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			ItoaBuf);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"\r\n");
	}
}

EFI_STATUS
EFIAPI uefi_main(EFI_HANDLE mImageHandle, EFI_SYSTEM_TABLE *mSystemTable)
{
	/* Overwrite this variable for status... */
	EFI_STATUS Result = EFI_SUCCESS;

	/* Allow anyone in the bootloader to see these. */
	ImageHandle = mImageHandle;
	SystemTable = mSystemTable;

	/* Terminate the watchdog timer. */
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	EFI_HANDLE *VideoBuffers = NULLPTR;
	CHAR16 ItoaBuf[32];

	/* Kernel entry point */
	VOID(FERALAPI * kern_init)
	(EfiBootInfo *) = NULLPTR;

	/* TODO: Get the ACPI tables from EFI */


	/* Get all the GOPs... */
	UINT64 NumDisplays = 0;
	Result = SystemTable->BootServices->LocateHandleBuffer(ByProtocol,
		&GuidEfiGraphicsOutputProtocol,
		NULL,
		&NumDisplays,
		&VideoBuffers);

	OpenProtocol = SystemTable->BootServices->OpenProtocol;

	if (Result != EFI_SUCCESS)
	{
		/*  TODO: Handle headless boot. */
		return Result;
	}
	/* Before anything else, get rid of annoying boot splash */
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

	WriteMessage(L"Starting Feralboot...\n");

	/* Get the locations for the video buffers...
	 * The kernel accepts an "array of structs" style pointer
	 * where the beginning, end, and display data are all defined.
	 */
	UINT_PTR *DisplayBuffers = NULLPTR;
	EfiAllocMaxAddr((sizeof(UINT_PTR) * NumDisplays * 3),
		&DisplayBuffers, 2 * 1024 * 1024, FALSE);
	SetupEfiFramebufferData(VideoBuffers, DisplayBuffers, NumDisplays);

	/* Check for protocols necessary to mess with filesystem. */
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULLPTR;
	Result = OpenProtocol(ImageHandle, &GuidEfiLoadedImageProtocol,
		(void **)(&LoadedImageProtocol), ImageHandle, NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"OpenProtocol had error (loaded image)...\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}

	EFI_SIMPLE_FILESYSTEM_PROTOCOL *FileSysProtocol = NULLPTR;
	Result = OpenProtocol(LoadedImageProtocol->DeviceHandle,
		&GuidEfiSimpleFSProtocol, (void **)(&FileSysProtocol),
		ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"OpenProtocol had error (simple FS)..\r\n");

		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}

	EFI_FILE_PROTOCOL *ESPRoot = NULLPTR;
	Result = FileSysProtocol->OpenVolume(FileSysProtocol, &ESPRoot);


	EFI_FILE_PROTOCOL *KernelImage = NULLPTR;
	Result = ESPRoot->Open(ESPRoot, &KernelImage,
		KernelLoc, EFI_FILE_MODE_READ,
		EFI_FILE_READ_ONLY);

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Kernel could not be initialized...\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}

	Result = ElfLoadFile(KernelImage, &kern_init);
	/* TODO: Keep EFI stuff in memory while this all happens... */
	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		for (int i = 0; i < INT32_MAX; ++i)
		{
		}
		SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown,
			EFI_SUCCESS, 0, NULLPTR);
	}

	InternalItoaBaseChange(kern_init, ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Got entry point: 0x");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");

	/* Close the file...  */
	KernelImage->Close(KernelImage);

	UINTN MapSize = 0;
	UINTN DescriptorSize = 0;

	UINTN MapKey = 0;
	UINT32 DescriptorVersion = 0;

	EFI_MEMORY_DESCRIPTOR *MemoryMap = NULLPTR;

	/* Get the UEFI memory stuff. */
	Result = SystemTable->BootServices->GetMemoryMap(&MapSize, MemoryMap,
		NULL, &DescriptorSize, NULL);

	if (Result != EFI_SUCCESS)
	{
		/* TODO... */
	}

	/* Add some area to actually hold the info. */
	MapSize += (DescriptorSize << 1);
	Result = EfiAllocMaxAddr(MapSize,
		(void **)(&MemoryMap),
		MAX_INIT_PAGED,
		FALSE);
	if (Result != EFI_SUCCESS)
	{
		/* TODO... */
	}

	Result = SystemTable->BootServices->GetMemoryMap(&MapSize, MemoryMap,
		&MapKey, &DescriptorSize, &DescriptorVersion);

	if (Result != EFI_SUCCESS)
	{
		/* TODO... */
	}

	UINT64 NumMemoryRanges = MapSize / DescriptorSize;
	EfiMemoryRange *MemoryRanges = NULLPTR;

	EfiAllocMaxAddr((sizeof(EfiMemoryRange) * NumMemoryRanges),
		(void **)&MemoryRanges, MAX_INIT_PAGED, FALSE);


	EfiBootInfo *EnvBlock = NULLPTR;
	/* Don't allocate above 2MB. */
	if (!EfiAllocMaxAddr(sizeof(EfiBootInfo),
		    (void **)&EnvBlock,
		    2 * 1024 * 1024,
		    FALSE))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Firmware ran out of memory. Trying anyway.\r\n");
		SystemTable->BootServices->Stall(125000);
	}

	InternalItoaBaseChange(EnvBlock, ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"EnvBlock: 0x");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");

	for (UINT64 Iterator = 0; Iterator < NumMemoryRanges; ++Iterator)
	{
		EFI_MEMORY_DESCRIPTOR *Current = &(MemoryMap[Iterator]);
		UINTN Size = EFI_PAGE_SIZE * (Current->NumberOfPages);

		UINTN Begin = Current->PhysicalStart;
		UINTN End = Current->PhysicalStart + Size;

		if (Current->Type == EfiRuntimeServicesCode
			|| Current->Type == EfiRuntimeServicesData)
		{
			/* FIXME: remap the stuff. Do this, use 
			   SetVirtualAddressMap, then ChangePointer on
			   the table, and then we're good to mess
			   with cr3. */
		}

		MemoryRanges[Iterator].Usable = GetEfiMemoryMapToFreeOrNot(Current);
		MemoryRanges[Iterator].Start = Begin;
		MemoryRanges[Iterator].End = End;
	}

	EnvBlock->NumMemoryRanges = NumMemoryRanges;
	EnvBlock->MemoryRanges = MemoryRanges;

	EnvBlock->NumDisplays = NumDisplays;
	EnvBlock->FramebufferPAddrs = DisplayBuffers;

	/* Terminate boot services (about to execute kernel) */
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Terminating firmware services...\r\n");

	/* Blank the screen once more, then call the kernel */
	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	for (UINT64 Row = 0; Row < DisplayBuffers[1]; ++Row)
	{
		for (UINT64 Col = 0; Col < DisplayBuffers[0]; ++Col)
		{
			/* Look a little pretty, and visually indicate boot
			 * was successful. TODO: Diable if -v was passed in */
			((UINT32 *)(DisplayBuffers[2]))
				[(Row * DisplayBuffers[0]) + Col]
				= 0x00161616;
		}
	}
	kern_init(EnvBlock);
	for (int i = 0; i < INT32_MAX; ++i)
	{
	}
	return EFI_SUCCESS;
}
