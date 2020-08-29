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

#ifndef _FERAL_KERNEL_VGA_H_
#define _FERAL_KERNEL_VGA_H_


#include <krnl.h>

#if defined(__cplusplus)
extern "C"
{
#endif


	typedef enum VgaColorValue
	{
		VGA_BLACK = 0,
		VGA_BLUE = 1,
		VGA_GREEN = 2,
		VGA_CYAN = 3,
		VGA_RED = 4,
		VGA_MAGENTA = 5,
		VGA_BROWN = 6,
		VGA_LIGHT_GREY = 7,
		VGA_DARK_GREY = 8,
		VGA_LIGHT_BLUE = 9,
		VGA_LIGHT_GREEN = 10,
		VGA_LIGHT_CYAN = 11,
		VGA_LIGHT_RED = 12,
		VGA_LIGHT_MAGENTA = 13,
		VGA_LIGHT_BROWN = 14, /* actually yellow */
		VGA_WHITE = 15,
	} VgaColorValue;

	/* Rework to look more like direction we're brining Feral API to.
	 * (redoing struct above) */

	typedef struct
	{
		UINT16* Framebuffer;

		UINT16* SwappedBuffer;

		UINT16 ScreenWidth;
		UINT16 ScreenHeight;

		VgaColorValue Background;
		VgaColorValue Foreground;
		VgaColorValue Highlight;

		UINT16 CurrentRow;
		UINT16 CurrentCol;

		BOOL TextMode;
		BOOL FollowingInput;
		BOOL CursorEnabled;
	} VgaContext;


	VOID KiBlankVgaScreen(DWORD height, DWORD width, DWORD color);

	VOID VgaPutChar(CHAR letter);

	VOID VgaEntry(VgaColorValue foreground, VgaColorValue background,
		CHAR letter, DWORD posx, DWORD posy);

	VOID VgaAutoEntry(VgaColorValue foreground, VgaColorValue background,
		CHAR letter);

	VOID VgaPutChar(CHAR letter);

	VOID VgaStringEntry(VgaColorValue foreground, VgaColorValue background,
		CHAR* string, DWORD length, DWORD posx, DWORD posy);

	VOID VgaPrint(VgaColorValue foreground, VgaColorValue background,
		CHAR* string, DWORD length);

	VOID VgaPrintln(VgaColorValue foreground, VgaColorValue background,
		CHAR* string, DWORD length);

	VOID VgaAutoPrintln(VgaColorValue Foreground, VgaColorValue Background,
		CHAR* String);

	VOID VgaAutoPrint(VgaColorValue Foreground, VgaColorValue Background,
		CHAR* String);

	VOID VgaMoveCursor(DWORD PosX, DWORD PosY);

	VOID VgaGetCurrentPosition(OUT UINT16* X, OUT UINT16* Y);

	VOID VgaSetCurrentPosition(IN UINT16 X, OUT IN UINT16 Y);

	VgaGetFramebufferDimensions(OUT UINT16* Width, OUT UINT16* Height);

	/* Whenever we set a character, set the position of the cursor to it
	 * + 1. */
	VOID VgaTraceCharacters(BOOL value);

	VOID VgaSetCursorEnabled(BOOL value);

	VOID VgaSwapBuffers(VOID);

	UINT8 VgaPrepareEnvironment();


#if defined(__cplusplus)
}
#endif


#endif
