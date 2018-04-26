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


#ifndef _WAYPOINT_RESOURCES_H_
#define _WAYPOINT_RESOURCES_H_

#include <feral/stdtypes.h>

// "Resources" here refers to System RAM + swapfile space.
// By default, all users have access to all RAM minus whatever's in use.
// Programs can be limited to use a subset of these resources, or have swapping enabled.
// If we run out of real RAM, swapfile permissions are ignored and programs have their pages swapped based on lowest priority.
// Otherwise, a program can ask to voluntarilly put some of itself into the swapfile, ie, save real RAM for programs that actually need it.
// (though I'd expect very few programs would actually use this feature.)

/* _all_ system resources left on the system */
#define SYSTEM_RESOURCES_ALL	(0x0000)

/* The resources the calling user has access to. This may or may not be equal to ALL. */
#define SYSTEM_RESOURCES_USER	(0x0001)

/* The resources this program is allowed to use. This may be less than what the user has access to. */
#define SYSTEM_RESOURCES_TASK	(0x0002)

/* The resources which this thread is allowed to use. This may be less than what the task has access to. */
#define SYSTEM_RESOURCES_THREAD (0x0004)

/**
	Get the type of resource that is allowed: see above definitions for valid input.
	@return The amount of RAM (real + swapfile) which the type is allowed to use.
 */
UINTN GetAvailableSystemResources(UINTN ResourceType);


/**
	Gets the endianness of the system.
	Returns 1 for big endian, and 0 for little endian.
	If this is ever ported to a platform which has a different endianness, returns -1.
 */
UINT32 GetSystemEndianness(void);

#endif
