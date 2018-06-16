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

#ifndef _LIBRE_EFI_EFI_COMMON_H_
#define _LIBRE_EFI_EFI_COMMON_H_


typedef WCHAR CHAR16;

typedef VOID* EFI_HANDLE;
typedef VOID* EFI_EVENT;
typedef UINTN EFI_TPL;
typedef UINT64 EFI_LBA;

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

#define MDE_CPU_X64	// In case any code relies upon that...

#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__ short
#endif

// #define HAVE_USE_MS_ABI 1	// Let's just not support old versions of GCC 
// since having some weird objcopy hack is bad
// I don't think I should bother with gnu-efi compatibility (since clang can make pe-coff anyway, so i don't need to borrow linker scripts and all)
// Probably should just get rid of that define altogether

#define EFI_ERROR_MASK 	(0x8000000000000000)
#define EFI_OEM_MASK   	(0xC000000000000000)
#define EFIERR(x) 	(EFI_ERROR_MASK | x)
#define EFIERR_OEM(x) 	(EFI_OEM_MASK | x)

#define BAD_POINTER (0xFBFBFBFBFBFBFBFBULL)
#define MAX_ADDRESS (0xFFFFFFFFFFFFFFFFULL)
#define MAX_2_BITS  (0xC000000000000000ULL)
#define MAX_BIT     (0x8000000000000000ULL)

#define MAX_INTN    ((INTN) 0x7FFFFFFFFFFFFFFFULL)
#define MAX_UINTN   ((UINTN)0xFFFFFFFFFFFFFFFFULL)




// On x86-64, the stack needs to be aligned with 16.
#if defined(__x86_64__)
#define CPU_STACK_ALIGNMENT (0x10)
#endif

// 4K pages...
#define DEFAULT_PAGE_ALLOCATION_GRANULARITY (0x1000)
#define RUNTIME_PAGE_ALLOCATION_GRANULARITY (0x1000)



#define EVT_TIMER 0x80000000
#define EVT_RUNTIME 0x40000000
#define EVT_NOTIFY_WAIT 0x00000100
#define EVT_NOTIFY_SIGNAL 0x00000200
#define EVT_SIGNAL_EXIT_BOOT_SERVICES 0x00000201
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

#define TPL_APPLICATION 0x04
#define TPL_CALLBACK 0x08
#define TPL_NOTIFY 0x10
#define TPL_HIGH_LEVEL 0x1F

#define OPTIONAL


typedef struct 
{
	UINT32 Data1;
	UINT16 Data2;
	UINT16 Data3;
	UINT8 Data4;
	UINT8 Data5;
	UINT8 Data6;
	UINT8 Data7;
	UINT8 Data8;
	UINT8 Data9;
	UINT8 Data11;
	UINT8 Data12;
}EFI_GUID;

// Needed for simple text input protocol...
#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID		\
{							\
	0x387477C1,					\
	0x69C7,						\
	0x11D2,						\
	0x8E,						\
	0x39						\
	0x00,						\
	0xA0,						\
	0xC9,						\
	0x69,						\
	0x72,						\
	0x3B,						\
}


// Needed for simple text output protocol... (no, this isn't a typo, they're the same.)
#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID		\
{							\
	0x387477c2,					\
	0x69C7,						\
	0x11D2,						\
	0x8E,						\
	0x39,						\
	0x00,						\
	0xA0,						\
	0xC9,						\
	0x69,						\
	0x72,						\
	0x3B,						\
}

/* EFI allocation enum. */

typedef enum EFI_ALLOCATE_TYPE
{
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateType,
}EFI_ALLOCATE_TYPE;

typedef enum
{
	TimerCancel,
	TimerPeriodic,
	TimerRelative
}EFI_TIMER_DELAY;

typedef struct
{
	UINT64 Signature;
	UINT32 Revision;
	UINT32 HeaderSize;
	UINT32 CRC32;
	UINT32 Reserved;
}EFI_TABLE_HEADER;

typedef struct
{
	UINT16 ScanCode;
	CHAR16 UnicodeChar;
}EFI_INPUT_KEY;

typedef struct
{
	UINT32 Type;
	EFI_PHYSICAL_ADDRESS PhysicalStart;
	EFI_VIRTUAL_ADDRESS VirtualStart;
	UINT64 NumberOfPages;
	UINT64 Attribute;
}EFI_MEMORY_DESCRIPTOR;

#define EFI_MEMORY_UC		(0x0000000000000001)
#define EFI_MEMORY_WC		(0x0000000000000002)
#define EFI_MEMORY_WT		(0x0000000000000004)
#define EFI_MEMORY_WB		(0x0000000000000008)
#define EFI_MEMORY_UCE		(0x0000000000000010)
#define EFI_MEMORY_WP		(0x0000000000001000)
#define EFI_MEMORY_RP		(0x0000000000002000)
#define EFI_MEMORY_XP		(0x0000000000004000)
#define EFI_MEMORY_RUNTIME	(0x8000000000000000)

typedef enum
{
	EFI_NATIVE_INTERFACE
}EFI_INTERFACE_TYPE;


#endif
