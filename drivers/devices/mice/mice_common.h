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
#ifndef _FERAL_MICE_COMMON_H_
#define _FERAL_MICE_COMMON_H_

#include <feral/stdtypes.h>
#include <ddk/frlddk.h>
#include <ddk/interface.h>

#if defined(__x86_64__) || defined(__i386__)
#include <arch/x86_64/cpuio.h>
#endif


// We want some common stuff for our peripherals, if they don't obey this, we translate it in the driver.
// Specifically, I'm looking for compatibility with my current mouse and keyboard, both of which can be controlled over USB.
// We don't have a USB stack yet, so we pretend we do for now and then lay down a framework to improve now.

#define MOUSE_LED_ON 1
#define MOUSE_LED_OFF 0

#endif