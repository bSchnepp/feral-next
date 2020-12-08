/*
Copyright (c) 2018 2020, Brian Schnepp

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


#include <feral/stdtypes.h>
#include <ddk/frlddk.h>
#include <ddk/interface.h>

#if defined(__x86_64__) || defined(__i386__)
#include <arch/x86_64/cpuio.h>
#endif


#include "serial.h"

/* We should probably call it by it's proper name, the i8000-something,
 * instead of just generically calling it "serial".
 */

VOID SerialConfigureBaudRate(UINT16 Port, UINT16 Divisor)
{
#if defined(__x86_64__) || defined(__i386__)
	x86outb(SERIAL_LINE_COMMAND(Port), SERIAL_LINE_ENABLE_DLAB);
	x86outb(Port, (Divisor >> 0) & 0xFF);
	x86outb(SERIAL_INTERRUPT_COMMAND(Port), (Divisor >> 8) & 0xFF);
#endif
}

VOID SerialSetInterrupts(UINT16 Port, UINT8 Bitmask)
{
#if defined(__x86_64__) || defined(__i386__)
	x86outb(SERIAL_FIFO_COMMAND(Port), Bitmask);
#endif
}

/**
 * Set up the serial port to have properties regarding
 * parity bits, data length, and break control.
 * @author Brian Schnepp
 * @param Port The port to configure
 * @param Bitmask The mask for how to configure the serial port.
 */
VOID SerialSetFlags(UINT16 Port, UINT8 Bitmask)
{
	x86outb(SERIAL_LINE_COMMAND(Port), Bitmask);
}

VOID SerialSetMode(UINT16 Port, UINT8 Data)
{
	x86outb(SERIAL_FIFO_COMMAND(Port), Data);
}


VOID SerialSend(UINT16 Port, STRING c, UINT64 Len)
{
#if defined(__x86_64__) || defined(__i386__)
	for (UINT64 Index = 0; Index < Len; ++Index)
	{
		if ((x86inb(SERIAL_LINE_STATUS(Port)) & (1 << 5)))
		{
			if (c[Index] == '\n')
			{
				x86outb(Port, '\r');	
			}
			x86outb(Port, c[Index]);
		}
	}
#endif
}

VOID SerialRecv(UINT16 Port, UINT64 Amt, BYTE *Buf)
{
#if defined(__x86_64__) || defined(__i386__)
	for (UINT64 Index = 0; Index < Amt; ++Index)
	{
		Buf[Index] = x86inb(Port);
	}
#endif
}

FERALSTATUS InitSerialDevice(VOID *OutData)
{
	KiPrintFmt("Initializing serial...\n");

#if defined(__x86_64__) || defined(__i386__)
	SerialSetInterrupts(COM1_PORT, 0);
	x86outb(SERIAL_LINE_COMMAND(COM1_PORT), SERIAL_LINE_ENABLE_DLAB);
	SerialConfigureBaudRate(COM1_PORT, 12);
	x86outb(SERIAL_INTERRUPT_COMMAND(COM1_PORT), 0);
	SerialSetFlags(COM1_PORT, 3);
	SerialSetMode(COM1_PORT, 0xC7);
	x86outb(SERIAL_MODEM_COMMAND(COM1_PORT), 0x0B);
#endif

	BYTE Item;
	SerialRecv(SERIAL_LINE_STATUS(COM1_PORT), 1, &Item);
	if (Item == 0xFF)
	{
		KiPrintFmt("%s\n", "Unable to initialize serial! (no port)");
		return STATUS_ERROR;
	}

	SerialSetInterrupts(COM1_PORT, 0x02);
	return STATUS_SUCCESS;
}
