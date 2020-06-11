/*
Copyright (c) 2019, Brian Schnepp

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

#ifndef _FERAL_MACH_MESSAGE_H_
#define _FERAL_MACH_MESSAGE_H_

#ifdef MACH_KERNEL

/* Deal with this later... */
#undef MACH_KERNEL
#endif

#include <mach/port.h>
#include <mach/kern_return.h>

/*
	Units are milliseconds: these are passed by value.
	This is controlled by MACH_SEND_TIMEOUT and MACH_RCV_TIMEOUT
	in version 3 of Mach. This might have been changed in version 4?
 */
typedef natural_t mach_msg_timeout_t;


#define MACH_MSG_TIMEOUT_NONE ((mach_msg_timeout_t)(0));

/* TODO: document better */
#define MACH_MSGH_BITS_ZERO (0x00000000)
#define MACH_MSGH_BITS_REMOTE_MASK (0x000000FF)
#define MACH_MSGH_BITS_LOCAL_MASK (0x0000FF00)
#define MACH_MSGH_BITS_COMPLEX (0x80000000)

/* TODO!!! */



#endif
