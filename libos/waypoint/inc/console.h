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

#ifndef _FERAL_CONSOLE_H_
#define _FERAL_CONSOLE_H_

// We follow POSIX with file IDs, but our names are different...
#define SYS$INPUT (0)
#define SYS$OUTPUT (1)
#define SYS$ERROR (2)

// Compatibility...
#define CONIN$ SYS$INPUT
#define CONOUT$ SYS$OUTPUT
#define CONERR$ SYS$ERROR

#ifndef stdin
#define stdin SYS$INPUT
#define stdout SYS$OUTPUT
#define stderr SYS$ERROR
#endif

typedef enum MouseType
{
	MOUSE_TYPE_LASER,// Basically everything today!
	MOUSE_TYPE_TRACKBALL,// pretty sure the one I had as a kid is long gone now (the acompanying PS/2 keyboard, for the record, was absolutely abysmal. Good riddance.)
	MOUSE_TYPE_THUMBBALL,// Those mice where the trackball was on the side.
	MOUSE_TYPE_TRACKPAD,// laptops
	MOUSE_TYPE_NUB,// Like on the X60 (a real, true IA-32 that's still 'modern' and can run the latest version of the (GNU) Mach kernel without buying expensive 64-bit CPU)...
	MOUSE_TYPE_UNREAL,// If we called this 'virtual', it would be confused with VMs. This is where the mouse is actually a laser projection and doesn't really "exist".
	MOUSE_TYPE_3D,// Kinda like the remote for a certain Power-based 7th generation console from Japan famous for motion tracking. (cant really be less specific than that :| ).
	MOUSE_TYPE_OTHER,// We don't know!
} MouseType;

typedef struct MOUSEHANDLE
{
	MouseType Type;
	//TODO...
} MOUSEHANDLE;

typedef struct KBRDHANDLE
{
	//TODO...
} KBRDHANDLE;


/**
	Returns a value based upon if the keyboard was recently hit or not.
	If true, then you should read the buffer, otherwise it may be overwritten.

	@return A value denoting if the keyboard was recently hit or not.
 */
BOOL KeyboardHit(KBRDHANDLE* HKeyboard);



#endif
