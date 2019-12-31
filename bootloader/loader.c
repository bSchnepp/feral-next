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

#include <libreefi/efi.h>
#include <feral/stdtypes.h>

#ifndef _FRLBOOT_NO_SUPPORT_ELF64_
#include <drivers/proc/elf/elf.h>
#else
#error "Unsupported compile mode..."
#endif

static EFI_HANDLE ImageHandle;
static EFI_SYSTEM_TABLE *SystemTable;


static EFI_GUID GuidEfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID GuidEfiSimpleFSProtocol = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID GuidEfiFileInfoGuid = EFI_FILE_INFO_ID;

/* TODO: Wrap in a struct marked static so kernel knows to protect it. */
static EFI_GUID GuidEfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;



/* TODO: Refactor into something nice. */

EFI_STATUS EFIAPI LdrReadBootInit(EFI_FILE_PROTOCOL *BootIni)
{
	return EFI_SUCCESS;
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
			Buf[Len++]  =  Rem + L'0';
		} else if (Rem < 35) {
			/* Encode the number as a lowercase letter. */
			Buf[Len++]  =  (Rem - 10) + L'a';
		} else {
			/* Encode as uppercase. */
			Buf[Len++]  =  (Rem - 36) + L'A';
		}
	}
	
	/* It's written BACKWARDS. So flip the order of the string. */
	for (ReverseIndex = 0; ReverseIndex < Len / 2; ++ReverseIndex)
	{
		CHAR16 Tmp = Buf[ReverseIndex];
		Buf[ReverseIndex] = Buf[Len - ReverseIndex  - 1];
		Buf[Len - ReverseIndex  - 1] = Tmp;
	}
	
	/* Terminate the string. */
	Buf[Len] =  '\0';
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
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}
	
	/* Clear the display. */
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
	SystemTable->ConOut->OutputString(SystemTable->ConOut, 
		L"Starting Feralboot...\r\n");
		
	OpenProtocol = SystemTable->BootServices->OpenProtocol;

	for (Iterator = 0; Iterator < NumDisplays; ++Iterator)
	{
		/* Get the current GOP. */
		Result = OpenProtocol(VideoBuffers[Iterator], 
			&GuidEfiGraphicsOutputProtocol, (VOID**)&GraphicsProtocol,
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
		(void**)(&MemoryMap)
	);
	if (Result != EFI_SUCCESS)
	{
		/* TODO... */
	}
	
	Result = SystemTable->BootServices->GetMemoryMap(&MapSize, MemoryMap, 
		&MapKey, &DescriptorSize, &DescriptorVersion
	);
	
	if (Result != EFI_SUCCESS)
	{
		/* TODO... */
	}

	Result = OpenProtocol(ImageHandle, &GuidEfiLoadedImageProtocol,
		(void**)(&LoadedImageProtocol), ImageHandle, NULL, 
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
		&GuidEfiSimpleFSProtocol, (void**)(&FileSysProtocol), 
		ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);

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
		EFI_FILE_READ_ONLY
	);
	
	if (Result != EFI_SUCCESS)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut, 
			L"Kernel could not be initialized...\r\n");
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
			EfiErrorToString(Result));
		SystemTable->BootServices->Stall(12500000);
		return Result;
	}
		
	
	/* Terminate boot services (about to execute kernel) */
	SystemTable->ConOut->OutputString(SystemTable->ConOut, 
		L"Terminating firmware services...\r\n");
		
	/* Terminate the watchdog timer. */
	SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
	
	/* Start loading of the kernel. (setup and call KiSystemStartup) */
	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	
	SystemTable->ConOut->OutputString(SystemTable->ConOut, 
		L"Kernel has exited.\r\n");
	SystemTable->BootServices->Stall(12500000);

	return EFI_SUCCESS;
}
