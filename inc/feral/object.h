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

#ifndef _FERAL_OB_H_
#define _FERAL_OB_H_

#include <feral/stdtypes.h>
#include <feral/string.h>
#include <feral/feraluser.h>
#include <feral/handle.h>

typedef enum FERAL_OBJECT_TYPE
{
	/* What kind of object are we looking at? */
	FERAL_FILE_OBJECT,
	FERAL_DRIVER_OBJECT,
	FERAL_WAVEFRONT_OBJECT,// for graphics stuff...
	FERAL_FRAME_OBJECT,
	FERAL_IMAGE_BUFFER_OBJECT,
	FERAL_DESKTOP_OBJECT,
	FERAL_CLIPBOARD_OBJECT,
	FERAL_PROGRAM_OBJECT,
	FERAL_SERVICE_OBJECT, /* A service is a userspace daemon which implements a specific set of functions requested by the kernel. (ie, a FUSE driver, or a printing service) */
	FERAL_PACKET_OBJECT, /* A network packet (9P, IP, etc.) */
	FERAL_NETWORK_OBJECT, /* Connection to some kind of network */
} FERAL_OBJECT_TYPE;

typedef struct FERAL_PERMISSION_DESCRIPTOR
{
	FERALUSER User;
	BOOLEAN Read;
	BOOLEAN Write;
	BOOLEAN Execute;
} FERAL_PERMISSION_DESCRIPTOR;

typedef enum FERAL_OBJECT_OPEN_REASON
{
	OBJECT_CREATE_HANDLE, /* First time referencing this object... */

	OBJECT_OPEN_HANDLE, /* Opening a reference to an already existing object */
	/* Sidenode: if it's been updated on the system (ie, overwritten), the OS delivers the old version to running programs and new versions to subsequent programs, ala Linux. */

	OBJECT_CLONE_HANDLE, /* Duplicated handle the program already has */

	OBJECT_INHERIT_HANDLE, /* The parent task opened this, and allowed it's children to use it. */
} FERAL_OBJECT_OPEN_REASON;

// Attributes for the descriptor below
#define OBJ_INHERIT (1 << 0)
#define OBJ_PERMANANT (1 << 1)
#define OBJ_EXCLUSIVE (1 << 2)
#define OBJ_CASE_INSENSITIVE (1 << 3)
#define OBJ_OPENIF (1 << 4)
#define OBJ_OPENLINK (1 << 5)
#define OBJ_KERNEL_HANDLE (1 << 6)

typedef struct OBJECT_ATTRIBUTES
{
	UINT64 Length;
	HANDLE RootDirectory;// TODO: Consider replacing (special type for object in RMS)
	WSTRING ObjectName;

	UINT64 NumReferences;// Increment on every open. Decrement on every close. If 0, free this object.

	FERALUSER Owner;// Kernel's name is "SYSTEM", UserID is 0, and it's home is A:/System/.
	UINT8 Attributes;

	UINT64 NumOfAuthorizedUsers;// Number of users who can access it
	FERALUSER* AuthorizedUsers;// And the array with the users allowed to use it
	FERAL_PERMISSION_DESCRIPTOR* Permissions;// With their corresponding permissions. Execute may or may not be applicable.
} OBJECT_ATTRIBUTES;


typedef struct ObjectFunction
{
	STRING FunctionName;
	FERALSTATUS (*FunctionReference)
	(IN UINT64 NumArgs, IN VOID** Stuff);
} ObjectFunction;

struct FERALOBJECT;

typedef struct FERALOBJECT
{
	/* Number of references to this object. If KeFreeObject() is called and makes this 0, 
	   the kernel should deallocate this object and free everything inside of it.
	 */

	STRING ObjectName;
	FERAL_OBJECT_TYPE ObjectType;

	UINT64 NumReferences;
	UINT64 MaxReferences;
	VOID* ReferenceTable;

	OBJECT_ATTRIBUTES* Attributes;

	UINT64 MaxMemorySize; /* Memory maximum */
	UINT64 DiskAllocMaximum; /* Max number of blocks on the filesystem this object can utilize. */

	BOOL Pageable; /* Can this object get thrown into swapfile if we need to? */

	UINT64 NumMethods;
	ObjectFunction* Methods;

	UINT64 RESERVED1;
	UINT64 RESERVED2;
	UINT64 RESERVED3;
	UINT64 RESERVED4;

	VOID* ObjectPointer; /* Pointer to the object in question. */
} FERALOBJECT;

typedef struct
{
	FERALOBJECT* Flink;
	FERALOBJECT* Blink;
} LIST_ENTRY;


#endif
