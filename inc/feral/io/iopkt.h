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

#ifndef FERAL_IOPKT_H_
#define FERAL_IOPKT_H_

#include <feral/stdtypes.h>
#include <feral/feralstatus.h>

#include <ddk/frlddk.h>



/* Definition of flags for the IRP. */
#define FLAG_CRITIAL 0xC000000000000000
#define FLAG_IMPORTANT 0x8000000000000000
#define FLAG_REQUEST 0x4000000000000000
#define FLAG_INFO 0x0000000000000000

typedef UINT64 IRPFlags;

/* Associated data */
#define REQUEST_TYPE_BINARY_READ 0x00000000
#define REQUEST_TYPE_BINARY_WRITE 0x00000001
#define REQUEST_TYPE_BINARY_COPY 0x00000002
#define REQUEST_TYPE_BINARY_DELETE 0x00000004
#define REQUEST_TYPE_BINARY_EXECUTE 0x00000008

#define REQUEST_TYPE_FILE_READ 0x00000010
#define REQUEST_TYPE_FILE_WRITE 0x00000011
#define REQUEST_TYPE_FILE_COPY 0x00000012
#define REQUEST_TYPE_FILE_DELETE 0x00000014
#define REQUEST_TYPE_FILE_EXECUTE 0x00000018

typedef UINT32 ReqFlags;

struct _IRP
{
	TIMEUNIT TimeSignature;
	IRPFlags Flags;

	union {
		struct _IRP* MasterIRP; /* We rely upon this one to go first
					   before we do something. */
		VOID* SystemMemoryBuffer; /* If we don't follow, we have some
					     area in RAM which is used to cache
					     this request before going through.
					   */
		/* Subsequent IRPs following will override the old memory buffer
		 * and become the new 'root IRP'. */
	} RootIrpLookup;

	BOOLEAN PendingRequest;
	BOOLEAN CancelRequest;

	FERALSTATUS*
		IoDriverPacketCancel; /* Optional function (pointer) that is
					 called when the IRP is cancelled. (This
					 can catch and resolve IO errors!!!) */

	FERAL_DRIVER* IoDiskDriver;

	ReqFlags RequestFlags;
	UINT8 Data[16]; /* We deal with 16 bytes at a time (128 bits). This may
			   be changed. */

	VOID* DiskLocation; /* TODO? */
};

typedef struct IRP IRP;
typedef IRP IORequest;

#endif
