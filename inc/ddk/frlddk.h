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

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>
#include <bogus/fluff.h>	//Maybe give this a more 'appropriate' name later or something. Thos definitions don't have a real purpose other than readability.

#if defined(__cplusplus)
extern "C" {
#endif


// TODO: these are STUBS.
// We need to replace this with a WORKING interface.
// Essentially, this is just pseudocode that compiles with clang.

/* Definitions for _layers_. ... what was I doing with this again??? */
#define FERAL_DRIVER_SPECIFIC_DEVICE	0x0F	/* Driver is SPECIFICALLY for this device. */
#define FERAL_DRIVER_SPECIFIC_FAMILY	0x0E	/* Driver is for this family of devices. */
#define FERAL_DRIVER_SPECIFIC_VENDOR	0x0D	/* Driver is likely to work for the device. */
#define FERAL_DRIVER_GENERIC_INTERFACE	0x0C	/* This implements generic functions(in software?) */
#define FERAL_DRIVER_GENERIC_COMMS	0x0B	/* Driver is an abstraction layer. (ie, vulkan) */
#define FERAL_DRIVER_EMULATED_DEVICE	0x0A	/* Imitate a device in software. */
#define FERAL_DRIVER_EMULATED_INTERFACE	0x09	/* Generic interface for software emulated devices. */

#define FERAL_DRIVER_FALLBACK		0x08	/* Fall back to this driver if we need to access something (ie video), but have no driver (ie, force VGA graphics) */
/* 0x00 - 0x07 are reserved for now. */

/* Flags. We get a whole UINT32 for this purpose. */
#define FERAL_DRIVER_FLAGS_FILESYSTEM	0x80000000
#define FERAL_DRIVER_FLAGS_GRAPHICS	0x40000000
#define FERAL_DRIVER_FLAGS_DISPLAY	0x20000000
#define FERAL_DRIVER_FLAGS_NETWORK	0x10000000



#define FERAL_DEVICE_CHARACTERISTICS_READ_ONLY	0x00
#define FERAL_DEVICE_CHARACTERISTICS_WRITE_ONLY	0x01
#define FERAL_DEVICE_CHARACTERISTICS_READ_WRITE	0x02
#define FERAL_DEVICE_CHARACTERISTICS_REMOVABLE	0x04
#define FERAL_DEVICE_CHARACTERISTICS_URSP_OPEN	0x08	/* Allow userspace to see this device located under A:/Devices/DEVICE_NAME, and access like a *NIX special file. */

// These structs will eventually moved out and extern'ed so that we can change the driver structure while being backwards compatible.

typedef struct LICENSE_IDENTIFIER
{
	STRING LicenseName;	// Please use 'Boost', 'GPLv2', 'GPLv3', 'MPLv2', 'MIT', 'BSD 4-clause', 'BSD 3-clause', "Proprietary", etc., and use 'DUAL:[<LICENSE1>,<LICENSE2>]' for dual-licenses.
				// This way, it'd be very easy for free software purists to cleanly identify exactly what is proprietary/closed-source and what isn't.
}LICENSE_IDENTIFIER;

typedef struct
{
	HANDLE	DeviceObject;
	STRING 	DeviceName;		/* Name of the device, for example "SUPERC00L 2900K" */
	UINT8	DeviceParameters;	/* How should we open this? (expose with the *NIXism of a file, but inside a kernel-only namespace. TODO on how to do this.) */
	UINT64	DeviceUserspaceDriver;	/* Figure out how to handle drivers in userspace here... (LONGWORD of process ID?) */
}DRIVER_OBJECT;

typedef struct
{
	/* The first argument is for the handle to the driver object in kernel-land, the second is for the handle to the device. */
	FERALSTATUS (*DriverInit)(HANDLE, HANDLE);
	FERALSTATUS (*DriverExit)(UINT64);
	
	FERALSTATUS (*DriverInvoke)(VOID*)	/* Communication to the driver is to be done here. TODO. */

	UINT32 	DriverFlags;
	UINT8	DriverPriority;	//Two nibbles, one for waiting priority, one for running proprity (allow a higher priveledge driver to directly control a device, instead of being fallback?)

}FERAL_DRIVER_FUNCTIONS;

typedef struct FERAL_DRIVER
{
	LICENSE_IDENTIFIER License;
	DRIVER_OBJECT Object;
	FERAL_DRIVER_FUNCTIONS Functions;
}FERAL_DRIVER;

/* The kernel will put all devices, even ones it doesn't recognize, into %FERAL_INTERNAL_NAMESPACE_ROOT%/System/Devices. These can be accessed through a HANDLE. */
FERALSTATUS KeCreateDriver(IN STRING License, IN HANDLE DeviceHandle, IN STRING DeviceName, OUT FERAL_DRIVER Driver);
FERALSTATUS KeModifyDriverParameters(INOUT FERAL_DRIVER Driver, IN UINT8 Parameters);
FERALSTATUS KeGetDriverParameters(IN FERAL_DRIVER Driver, OUT UINT8 Parameters);

/* TODO */
FERALSTATUS KeSetDriverFunctions(INOUT FERAL_DRIVER Driver);


#if defined(__cplusplus)
}
#endif
