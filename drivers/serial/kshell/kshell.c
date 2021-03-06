/*
Copyright (c) 2020, 2021, Brian Schnepp

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

#include <serial.h>

#include <feral/stdtypes.h>
#include <feral/kern/krnlfuncs.h>


CONST STRING Prompt = "frldbg> ";
CONST UINT64 PromptLen = 8;
CONST STRING Commands[] = {
	"!numa",
	"!paging",
	"!job",
	"!task",
	"!thread",
};

static CHAR CmdBuffer[1024];

VOID SerialDoKShellPrompt(VOID);

VOID SerialDoKShellPrompt(VOID)
{
	SerialSend(COM1_PORT, Prompt, PromptLen);
	KiSetMemoryBytes(CmdBuffer, 0, 1024);
	UINT16 Index = 0;
	BYTE CurChar = '\0';
	while (CurChar != '\n')
	{
		SerialRecv(COM1_PORT, 1, &CurChar);
		CmdBuffer[Index] = CurChar;
		if (Index != '\0')
		{
			Index++;
		}
		Index %= 1024;
	}

	/* parse it or something. */
}