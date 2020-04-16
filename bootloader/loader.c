/*
Copyright (c) 2018, 2019, 2020, Brian Schnepp

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
#error "Unsupported compile mode..."
#endif


#define EFI_PAGE_SIZE (4096)
#define EFI_PAGE_SHIFT (12)
#define EFI_PAGE_MAP (~(EFI_PAGE_SIZE - 1))
#define FERAL_VIRT_OFFSET (0xFFFFFFFFC0000000)


#if 0
For reference:

Reserved memory (don't touch it):
EfiReservedMemoryType
EfiRuntimeServicesCode
EfiRuntimeServicesData
EfiMemoryMappedIO
EfiMemoryMappedIOPortSpace
EfiPalCode

Bad memory:
EfiUnusableMemory

ACPI Reclaim:
EfiACPIReclaimMemory

Usable ("free"):
EfiLoaderCode
EfiLoaderData
EfiBootServicesCode
EfiBootServicesData
EfiConventionalMemory

ACPI NVM:
EfiACPIMemoryNVS

#endif

static EFI_HANDLE ImageHandle;
static EFI_SYSTEM_TABLE *SystemTable;

static EFI_GUID GuidEfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID GuidEfiSimpleFSProtocol = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID GuidEfiFileInfoGuid = EFI_FILE_INFO_ID;

/* TODO: Wrap in a struct marked static so kernel knows to protect it. */
static EFI_GUID GuidEfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;


static EfiBootInfo *EnvBlock = NULLPTR;

static UINT64 VirtMemAreaFree = 0;
static EFI_RUNTIME_SERVICES *VirtRuntimeTable = NULLPTR;

EFI_STATUS EFIAPI FeralBootProtocolRemap(VOID);
UINT64 BytesToEfiPages(UINT64 Bytes);
VOID InternalItoaBaseChange(UINT64 Val, CHAR16 *Buf, UINT8 Radix);


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

#ifndef _FRLBOOT_NO_SUPPORT_ELF64_
EFI_STATUS EFIAPI ElfLoadFile(IN EFI_FILE_PROTOCOL *File, OUT VOID **Entry)
{
	/* TODO */
	ElfHeader64 InitialHeader;
	ElfProgramHeader64 ProgramHeader;

	EFI_PHYSICAL_ADDRESS PAddress;
	EFI_STATUS Status;

	UINT64 Index;
	UINT64 Subindex;
	UINT64 Size = sizeof(ElfHeader64);
	UINT64 PHdrSize = sizeof(ElfProgramHeader64);

	/* Extra space, just to be safe. */
	CHAR16 ItoaBuf[32];

	Status = File->Read(File, &Size, &InitialHeader);
	if (Status != EFI_SUCCESS)
	{
		return Status;
	}

	/* Check the header... */
	CHAR Match[4] = {0x7F, 'E', 'L', 'F'};
	for (Index = 0; Index < 4; ++Index)
	{
		if (InitialHeader.magic[Index] != Match[Index])
		{
			return EFI_LOAD_ERROR;
		}
	}

	if (InitialHeader.e_machine != MACHINE_ID_NONE
		&& InitialHeader.e_machine != CUR_ARCH_ELF)
	{
		return EFI_LOAD_ERROR;
	}

	if (InitialHeader.e_phnum == 0)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Not an executable: missing program headers... \r\n");
		return EFI_LOAD_ERROR;
	}

	/* Change the position to the area currently cared about. */
	for (Index = 0; Index < InitialHeader.e_phoff; ++Index)
	{
		/* Read the program data in... */
		File->SetPosition(File, InitialHeader.e_phoff
						+ InitialHeader.e_phentsize * Index);

		/* Read it in... */
		File->Read(File, &PHdrSize, &ProgramHeader);

		if (ProgramHeader.p_type == PT_LOAD)
		{
			/* PAddr is in multiples of 4096. */
			PAddress = ProgramHeader.p_paddr;

			Status = SystemTable->BootServices->AllocatePages(
				AllocateAddress, EfiLoaderData,
				BytesToEfiPages(ProgramHeader.p_memsz),
				&PAddress);
			InternalItoaBaseChange(BytesToEfiPages(ProgramHeader.p_memsz), ItoaBuf, 10);
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				L"Attempting to allocate: ");
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				ItoaBuf);
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				L" pages at location 0x");
			/* Correct wrong address */
			InternalItoaBaseChange(BytesToEfiPages(ProgramHeader.p_paddr << 12), ItoaBuf, 16);
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				ItoaBuf);
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
				L"\r\n");

			/* Ignore EFI not doing it's job. (EFI_NOT_FOUND) */
			if (Status != EFI_SUCCESS)
			{
				if (Status == EFI_NOT_FOUND)
				{
					/* warn the user that bios is buggy */
					SystemTable->ConOut->OutputString(SystemTable->ConOut,
						L"[WARNING]: BIOS bug being suppressed with brute force.\r\n");
					SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Ask manufacturer for fix.\r\n");
				}
				else
				{
					SystemTable->ConOut->OutputString(SystemTable->ConOut,
						L"Failed to get address: ");
					InternalItoaBaseChange(PAddress, ItoaBuf, 16);
					SystemTable->ConOut->OutputString(SystemTable->ConOut,
						ItoaBuf);
					SystemTable->ConOut->OutputString(SystemTable->ConOut,
						L"\r\n");
					return Status;
				}
			}

			/* Zero it all out first... */
			for (Subindex = 0; Subindex < ProgramHeader.p_memsz;
				++Subindex)
			{
				((CHAR *)(PAddress))[Subindex] = '\0';
			}
			/* Read all the data in. */
			Status = File->SetPosition(File, ProgramHeader.p_offset);

			if (Status != EFI_SUCCESS)
			{
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					L"Failed to setup kernel (file seek).\r\n");
				return Status;
			}
			UINTN PFilesz = ProgramHeader.p_filesz;

			Status = File->Read(File, &(PFilesz), (VOID *)(PAddress));
			if (Status != EFI_SUCCESS || *(CHAR *)(PAddress) == 0)
			{
				SystemTable->ConOut->OutputString(SystemTable->ConOut,
					L"Failed to load program section.\r\n");
			}
		}
	}
	/* Update this so that uefi_main is happy. */
	VirtMemAreaFree = PAddress + ProgramHeader.p_filesz + 4096;
	VirtMemAreaFree += FERAL_VIRT_OFFSET;
	/* align to 4k */
	VirtMemAreaFree = VirtMemAreaFree & EFI_PAGE_MAP;

	/* Entry is already in vaddr. */
	*Entry = InitialHeader.e_entry;
	return EFI_SUCCESS;
}
#endif

EFI_STATUS EFIAPI LdrReadBootInit(EFI_FILE_PROTOCOL *BootIni)
{
	return EFI_SUCCESS;
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



EFI_STATUS EFIAPI uefi_main(EFI_HANDLE mImageHandle, EFI_SYSTEM_TABLE *mSystemTable)
{
	UINTN MapSize = 0;
	UINTN MapKey = 0;
	UINTN DescriptorSize = 0;
	UINT32 DescriptorVersion = 0;
	UINTN NumDisplays = 0;
	UINTN Iterator = 0;
	UINT32 CurrentWidth = 0;
	UINT32 CurrentHeight = 0;

	/* Extra space, just to be safe. */
	CHAR16 ItoaBuf[32];

	EFI_STATUS Result = EFI_SUCCESS;
	EFI_MEMORY_DESCRIPTOR *MemoryMap = NULLPTR;

	EFI_FILE_PROTOCOL *KernelImage = NULLPTR;
	EFI_FILE_PROTOCOL *ESPRoot = NULLPTR;

	EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULLPTR;
	EFI_SIMPLE_FILESYSTEM_PROTOCOL *FileSysProtocol = NULLPTR;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsProtocol = NULLPTR;

	EFI_HANDLE *VideoBuffers = NULLPTR;

	EFI_OPEN_PROTOCOL OpenProtocol;

	VOID(FERALAPI * kern_init)
	(EfiBootInfo *) = NULLPTR;


	ImageHandle = mImageHandle;
	SystemTable = mSystemTable;

	/* Get all the GOPs... */
	Result = SystemTable->BootServices->LocateHandleBuffer(ByProtocol,
		&GuidEfiGraphicsOutputProtocol, NULL, &NumDisplays, &VideoBuffers);

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Unable to get displays...\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		/* Still allow headless boot... */
	}

	/* Clear the display. */
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Starting Feralboot...\r\n");

	OpenProtocol = SystemTable->BootServices->OpenProtocol;

	/* cleanup later... */
	UINT_PTR *DisplayBuffers = NULLPTR;
	SystemTable->BootServices->AllocatePool(EfiRuntimeServicesData,
		(sizeof(UINT_PTR) * NumDisplays * 3),
		&DisplayBuffers);

	/* Seems like it's 1 more than supposed to */
	for (Iterator = 0; Iterator < NumDisplays - 1; ++Iterator)
	{
		/* Get the current GOP. */
		Result = OpenProtocol(VideoBuffers[Iterator],
			&GuidEfiGraphicsOutputProtocol, (VOID **)&GraphicsProtocol,
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

		Result = GraphicsProtocol->SetMode(GraphicsProtocol, PixelRedGreenBlueReserved8BitPerColor);
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
		CurrentWidth = GraphicsProtocol->Mode->Info->HorizontalResolution;
		CurrentHeight = GraphicsProtocol->Mode->Info->VerticalResolution;

		/* And tell the user that we found it! */
		InternalItoaBaseChange(CurrentWidth, ItoaBuf, 10);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"Got display of width: ");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			ItoaBuf);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L" and height of ");
		InternalItoaBaseChange(CurrentHeight, ItoaBuf, 10);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			ItoaBuf);
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"\r\n");

		/* HACK: first and second are sizes.*/
		DisplayBuffers[Iterator + 0] = CurrentWidth;
		DisplayBuffers[Iterator + 1] = CurrentHeight;
		DisplayBuffers[Iterator + 2] = GraphicsProtocol->Mode->FrameBufferBase;
	}


	/* Get the UEFI memory stuff. */
	Result = SystemTable->BootServices->GetMemoryMap(&MapSize, MemoryMap,
		NULL, &DescriptorSize, NULL);

	if (Result == EFI_BUFFER_TOO_SMALL)
	{
		/* TODO... */
	}

	/* Add some area to actually hold the info. */
	MapSize += (DescriptorSize << 1);
	Result = SystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize,
		(void **)(&MemoryMap));
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

	/* Check for protocols necessary to mess with filesystem. */
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



	/* Load /EFI/Feral/FERALKER.NEL to do things! */

	Result = FileSysProtocol->OpenVolume(FileSysProtocol, &ESPRoot);

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			L"ESP could not be initialized...\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}



	Result = ESPRoot->Open(ESPRoot, &KernelImage,
		L"\\EFI\\Feral\\FERALKER.NEL", EFI_FILE_MODE_READ,
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

	/* Terminate the watchdog timer. */
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

	Result = ElfLoadFile(KernelImage, &kern_init);
	InternalItoaBaseChange(kern_init, ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Got entry point: 0x");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");

	InternalItoaBaseChange(DisplayBuffers[2], ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Framebuffer: 0x");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");



	/* TODO: Keep EFI stuff in memory while this all happens... */

	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(125000);
		SystemTable->RuntimeServices->ResetSystem(EfiResetShutdown,
			EFI_SUCCESS, 0, NULLPTR);
	}

	/* Close the file...  */
	KernelImage->Close(KernelImage);

	/* ehhhhh */
	UINT64 NumMemoryRanges = MapSize / DescriptorSize;
	EfiMemoryRange *MemoryRanges = NULLPTR;
	SystemTable->BootServices->AllocatePool(EfiRuntimeServicesData,
		(sizeof(EfiMemoryRange) * NumMemoryRanges),
		(void **)&MemoryRanges);

	SystemTable->BootServices->AllocatePool(EfiRuntimeServicesData,
		(sizeof(EfiBootInfo)),
		(void **)&EnvBlock);

	InternalItoaBaseChange(EnvBlock, ItoaBuf, 16);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"EnvBlock: 0x");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		ItoaBuf);
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"\r\n");

	/* Iterate through the memory map one more time. */
	for (Iterator = 0; Iterator < NumMemoryRanges; ++Iterator)
	{
		/* FIXME: Incompatible with C89 */
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

	/* Terminate boot services (about to execute kernel) */
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
		L"Terminating firmware services...\r\n");
	/* Start loading of the kernel. (setup and call KiSystemStartup) */
	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);

	/* Put the table at the end. */
	VirtRuntimeTable = (EFI_RUNTIME_SERVICES *)(VirtMemAreaFree);

	EnvBlock->NumDisplays = NumDisplays;
	EnvBlock->FramebufferPAddrs = DisplayBuffers;

	EnvBlock->NumMemoryRanges = NumMemoryRanges;
	EnvBlock->MemoryRanges = MemoryRanges;

	for (int k = 0; k < EnvBlock->FramebufferPAddrs[0]; ++k)
	{
		for (int z = 0; z < EnvBlock->FramebufferPAddrs[1]; ++z)
		{
			((UINT32 *)(EnvBlock->FramebufferPAddrs[2]))[k] = 0x00FFFF00;
		}
	}
	kern_init(EnvBlock);
	for (int k = 0; k < EnvBlock->FramebufferPAddrs[0]; ++k)
	{
		((UINT32 *)(EnvBlock->FramebufferPAddrs[2]))[k] = 0x00FF0000;
	}

	return EFI_SUCCESS;
}
