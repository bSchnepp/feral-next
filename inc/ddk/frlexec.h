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

#if defined(__cplusplus)
extern "C"
{
#endif

	typedef enum
	{
		FORMAT_TYPE_ELF = 0x0000,
		FORMAT_TYPE_COFF,
		FORMAT_TYPE_PE,
		FORMAT_TYPE_MACHO,
		FORMAT_TYPE_LE,// osFree executable format
		FORMAT_TYPE_NE,
		FORMAT_TYPE_MZ,// Legacy *-DOS executable
		FORMAT_TYPE_AOUT,
		FORMAT_TYPE_COM,
		FORMAT_TYPE_OTHER = 0xFFFF,
	} ExecutableFormat;

	typedef struct
	{
		FERALSTATUS(*LoaderMain)
		(IN ExecutableFormat Format);
		FERALSTATUS(*LoaderExit)
		(VOID);

	} FormatHandler;

	FERALSTATUS KeAttachHandler(IN FERALSTATUS(*LoaderMain), IN ExecutableFormat Format);// Expect this to change...



#if defined(__cplusplus)
}
#endif
