/*
Copyright (c) 2018, 2019, 2021, Brian Schnepp

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

#ifndef _FERAL_FERALSTATUS_H_
#define _FERAL_FERALSTATUS_H_

#include <feral/stdtypes.h>

/**
 * @file feral/feralstatus.h
 * @brief A series of definitions for error handling and status for the kernel.
 */

#if defined(__cplusplus)
/* clang-format: off */
extern "C"
{
/* clang-format: on */
#endif

	typedef enum FERALSTATUS
	{
		STATUS_SEVERITY_SUCCESS = 0x00000000,
		STATUS_SEVERITY_INFORMATIONAL = 0x40000000,
		STATUS_SEVERITY_WARNING = 0x80000000,
		STATUS_SEVERITY_ERROR = 0xC0000000,

		STATUS_SUCCESS = STATUS_SEVERITY_SUCCESS,

		STATUS_INFORMATIONAL = STATUS_SEVERITY_INFORMATIONAL,
		STATUS_WAS_UNLOCKED
		= STATUS_SEVERITY_INFORMATIONAL | 0x00000017,

		STATUS_WARNING = STATUS_SEVERITY_WARNING,
		STATUS_UNAUTHORIZED_MEMORY_LOCATION
		= STATUS_SEVERITY_WARNING | 0x000000A0,
		STATUS_UNSUPPORTED_OPERATION
		= STATUS_SEVERITY_WARNING | 0x0000F0A1,

		STATUS_ERROR = STATUS_SEVERITY_ERROR,
		STATUS_STACK_GUARD_VIOLATION
		= STATUS_SEVERITY_ERROR | 0x00000001,
		STATUS_MEMORY_ACCESS_VIOLATION
		= STATUS_SEVERITY_ERROR | 0x00000005,
		STATUS_MEMORY_PAGE_FAILURE = STATUS_SEVERITY_ERROR | 0x00000006,
		STATUS_MEMORY_PAGE_CONFLICT
		= STATUS_SEVERITY_ERROR | 0x00000007,
		STATUS_WRONG_VOLUME = STATUS_SEVERITY_ERROR | 0x00000012,
		STATUS_NO_MEDIA_IN_DEVICE = STATUS_SEVERITY_ERROR | 0x00000013,
		STATUS_UNRECOGNIZED_MEDIA = STATUS_SEVERITY_ERROR | 0x00000014,
		STATUS_NONEXISTENT_SECTOR = STATUS_SEVERITY_ERROR | 0x00000015,

		STATUS_OUT_OF_MEMORY = STATUS_SEVERITY_ERROR | 0x00000017,
		STATUS_INVALID_MEMORY_LOCATION
		= STATUS_SEVERITY_ERROR | 0x000000A0,

		STATUS_INVALID_OPCODE = STATUS_SEVERITY_ERROR | 0x00080001,

	} FERALSTATUS;


	/* Gets a STRING representation of an error. */
	STRING KiGetErrorType(IN FERALSTATUS Status);

#if defined(__cplusplus)
	/* clang-format: off */
}
/* clang-format: on */
#endif

#endif
