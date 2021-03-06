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


#ifndef _FERAL_PRIVATE_HEADER_KRNL_H_
#define _FERAL_PRIVATE_HEADER_KRNL_H_

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>
#include <mm/mm.h>


#define VALIDATE_SUCCESS(x) \
	if (x != STATUS_SUCCESS) \
	{ \
		KiStopError(x); \
	}

typedef enum FeralStructureType
{
	FERAL_STRUCTURE_TYPE_APPLICATION_INFO = 0,
	FERAL_STRUCTURE_TYPE_APPLICATION_CREATE_INFO = 1,
	FERAL_STRUCTURE_TYPE_PORT_CREATE_INFO = 2,
	FERAL_STRUCTURE_TYPE_PORT_INFO = 3,
	FERAL_STRUCTURE_TYPE_GRAPHICS_CONTEXT_CREATE_INFO = 4,
	FERAL_STRUCTURE_TYPE_GRAPHICS_CONTEXT_INFO = 5,

} FeralStructureType;

// What the processor we're controlling is doing. (ie, no tasks attached to
// them)
typedef enum PROCESSOR_STATE
{
	ProcessorStateActive,
	ProcessorStateIdle,
} PROCESSOR_STATE;

// What each running program is doing.
typedef enum TASK_STATE
{
	TaskStateActive,
	TaskStateInactive,
	TaskStateActiveInSwap,
	TaskStateInactiveInSwap,
	TaskStateTransitionToSwap,
	TaskStateTransitionActivity,
} TASK_STATE;

typedef enum THREAD_STATE
{
	ThreadStateBlocking,
	ThreadStateRunning,
	ThreadStateInactive
} THREAD_STATE;

// Currently just a stub.
FERALSTATUS KiLoadAllDrivers(VOID);


typedef struct KTIMER
{
	FERALTIME InterruptTime;

} KTIMER;


#endif
