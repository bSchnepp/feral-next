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

#ifndef _FERAL_DRIVERS_GPU_GLOBAL_VIDMODE_H_
#define _FERAL_DRIVERS_GPU_GLOBAL_VIDMODE_H_

#include <feral/stdtypes.h>
#include <feral/feralstatus.h>

// ALL OF THESE ARE TODO!!!

typedef struct
{
	FERALSTATUS (*VidAddDevice)(VOID);		// Device detected.
	FERALSTATUS (*VidStartDevice)(VOID);		// Initialize the device (ie, load microcode or something?)
	FERALSTATUS (*VidStopDevice)(VOID);		// Stop the device (usually done for shutdown)
	FERALSTATUS (*VidRestartDevice)(VOID);		// Restart the driver. (Typically called when driver crashes, or updating drivers.)
	FERALSTATUS (*VidRemoveDevice)(VOID);		// Invoked when the graphics device is removed somehow. (ie, video over USB or something, or someone did a stupid and removed their PCIe GPU.)

	// TODO (will probably have some *massive* redesign too).
	
}GpuCoreFunctions;	// Function pointers are FUN!

typedef struct
{
	/* Return width, height, and (optionally) depth. */
	FERALSTATUS (*VidGetResolution)(OUT UINT32, OUT UINT32, OUTOPT UINT32);

	/* Get the location the video output is buffered to (if at all, we could be using the GPU as a compute device). */
	FERALSTATUS (*VidGetMemoryBuffer)(OUTOPT UINTN);
}VidBasicFunctions;

#endif
