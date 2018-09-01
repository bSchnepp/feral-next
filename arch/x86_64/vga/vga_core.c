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

#include <stdint.h>

#include <feral/feralstatus.h>
#include <feral/stdtypes.h>

#include <arch/x86_64/vga/vga.h>
#include <arch/x86_64/vga/vgaregs.h>
#include <arch/x86_64/cpuio.h>



volatile UINT16* VGA_LOC = (UINT16*)0xB8000;
static int VGA_CURRENT_LINE = 3;	// We directly write to the first 2 (and want an extra space), so leave space.
BOOL TraceVga;

VOID VgaEntry(VgaColorValue foreground, VgaColorValue background, CHAR letter, DWORD posx, DWORD posy)
{
	uint16_t color = ((background << 4) | foreground);
	VGA_LOC[posx + (posy * 80)] = ((UINT16)letter | (UINT16) color << 8);
}

VOID KiBlankVgaScreen(DWORD height, DWORD width, DWORD color)
{
	for (DWORD h = 0; h < height; h++)
	{
		for (DWORD w = 0; w < width; w++)
		{
			VgaEntry(color, color, (CHAR)(0), w, h);
		}
	}
}

VOID VgaStringEntry(VgaColorValue foreground, VgaColorValue background, CHAR* string, DWORD length, DWORD posx, DWORD posy)
{
	for (DWORD index = 0; index < length; ++index)
	{
		int true_y = posy + (index / 80);
		int true_x = index % 80;
		VgaEntry(foreground, background, string[index], true_x, true_y);
	}
	VGA_CURRENT_LINE += (length / 80);
}


VOID VgaPrintln(VgaColorValue foreground, VgaColorValue background, CHAR* string, DWORD length)
{
	if (VGA_CURRENT_LINE == 25)	// This is hardcoded for now, we'll change this later.
	{
		for (UINT32 i = 0; i < 25; i++)
		{
			for (UINT32 k = 0; k < 25; k++)
			{
				VGA_LOC[k + (i * 80)] = VGA_LOC[k + ((i + 1) * 80)];	//Copy everything over.
			}
		}
		VgaStringEntry(foreground, background, string, length, 0, VGA_CURRENT_LINE - 1);
	}
	else
	{
		VgaStringEntry(foreground, background, string, length, 0, VGA_CURRENT_LINE);
		VGA_CURRENT_LINE++;
	}
}

VOID VgaMoveCursor(DWORD PosX, DWORD PosY)
{
	UINT16 FinalPos = (UINT16)((PosY * 80) + PosX);
	x86outb(VGA_FB_COMMAND_PORT, VGA_LOW_BYTE_COMMAND);
	x86outb(VGA_FB_DATA_PORT, (UINT8)((FinalPos) & (0x00FF)));

	x86outb(VGA_FB_COMMAND_PORT, VGA_HIGH_BYTE_COMMAND);
	x86outb(VGA_FB_DATA_PORT, (UINT8)((FinalPos >> 8) & (0x00FF)));
}

VOID VgaTraceCharacters(BOOL value)
{
	TraceVga = value;
}

VOID VgaSetCursorEnabled(BOOL value)
{
	if (value)
	{
		x86outb(VGA_FB_COMMAND_PORT, 0x0A);
		x86outb(VGA_FB_DATA_PORT, (x86inb(VGA_FB_DATA_PORT) & 0xC0) | 0x00);	//OK, this is complex to explain. Just trust what I'm doing doesn't blow up GPUs.

		x86outb(VGA_FB_COMMAND_PORT, 0x0B);
		x86outb(VGA_FB_DATA_PORT, (x86inb(0x3E0) & 0xE0) | 0x0F);
	}
	else
	{
		x86outb(VGA_FB_COMMAND_PORT, 0x0A);
		x86outb(VGA_FB_DATA_PORT, 0x20);
	}
}

UINT8 VgaPrepareEnvironment(VOID)
{
	// Ensure a bit in port 0x03C2 is set.
	UINT8 miscreg = x86inb(0x3CC);
	x86outb(VGA_MISC_OUTPUT_REG, (miscreg | 0xE7));
	return miscreg;
}
