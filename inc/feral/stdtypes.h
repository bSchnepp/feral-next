/*
Copyright (c) 2018, Brian Schnepp

Permission is hereby granted, free of charge, to any person or organization 
obtaining  a copy of the software and accompanying documentation covered by 
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

#ifndef _FERAL_FERAL_STDTYPES_H_
#define _FERAL_FERAL_STDTYPES_H_

// I don't use C++ in the kernel because, well, it's the kernel. I don't want to set up exceptions and all that,
// and would much rather not be limited to a very small subset of the language.
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

//TODO: do this properly for other archs.
#if 0
typedef uint64_t natural_t;
typedef int64_t  integer_t;

typedef uint32_t uint32;
typedef  int32_t  int32;
#endif	//Can we just drop the legacy stuff instead of redefining datatypes over and over and over...?

/* ok, these aren't necessary, just to keep the ALL CAPS style that we're using for legacy reasons. */
#define CONST const
#define INLINE inline
#define VOID void
#define PVOID void*
// Reimplement ReactOS data types as needed so it's easier to port programs to FERAL WAYPOINT.
// The majority of desktop games run on a ReactOS-like environment... for backwards compatibility,
// we need these, even if the design choices are or aren't good/ugly/awful/whatever: that's 
// irrelevent, we just care about actually having ports. (kind of hard to test performance gains
// if we can't recompile the same program with minor modifications to test, no?)

// probably improperly using an ellipsis, whatever, don't care.

typedef unsigned char BYTE;
typedef BYTE BOOLEAN;
typedef char CCHAR;
typedef char CHAR;

typedef uint32_t DWORD;
typedef uint64_t DWORDLONG;

typedef uint32_t DWORD32;
typedef uint64_t DWORD64;

typedef float FLOAT;
typedef double DOUBLE;

typedef int32_t INT;
typedef INT BOOL;
typedef uint16_t WORD;

typedef PVOID HANDLE;
typedef HANDLE* PHANDLE;
typedef int HFILE;
typedef HANDLE HINSTANCE;
typedef HANDLE HKEY;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef unsigned char UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef int32_t LONG;
typedef int64_t LONGLONG;


/* I'll never understand the purpose of this over just 'UINTN', but OK. */
#if defined(__x86_64__)
typedef int64_t INT_PTR;
#elif defined(__i386__)
typedef int32_t INT_PTR;
#endif

typedef INT_PTR LONG_PTR;

typedef uint64_t QUAD;
typedef QUAD QWORD;

#if defined(__x86_64__)
typedef uint64_t UINTN;
#elif defined(__i386__)
typedef uint32_t UINTN;	
#endif
// 286 and earlier not supported.

// Ensure we define wchar_t. This is important, as we *really* love Unicode.
typedef unsigned short wchar_t;
typedef wchar_t* WSTRING;
typedef wchar_t WCHAR;
// Also define WIDE CHAR and WIDE STRING.

typedef CHAR* STRING;

typedef DWORD COLORREF;	/* For when we eventually get to a desktop environment. */
			/* We may need to bump this up to a higher bit count for better color stuff. */



typedef int16_t	CSHORT;

typedef UINT64 TIMEUNIT;

//Please avoid using this. (some archs don't like packed structs)
#define PACKED __attribute__((packed))


#ifndef NULL
#define NULL ((void*)0)
#endif

#if defined(__cplusplus)
}
#endif

#endif
