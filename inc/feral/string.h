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

#ifndef _FERAL_FERAL_STRING_H_
#define _FERAL_FERAL_STRING_H_

// I don't use C++ in the kernel because, well, it's the kernel. I don't want to set up exceptions and all that,
// and would much rather not be limited to a very small subset of the language.
#if defined(__cplusplus)
extern "C"
{
#endif

#include <feral/stdtypes.h>
#include <feral/feralstatus.h>


	/* Struct to define a verbose string (O(1) to get the length of the string) */
	typedef struct FERALSTRING
	{
		UINT64 Length;
		WSTRING Content;
	} FERALSTRING;

	/* Inline functions to do string comparison. We do NOT want the overhead of a function call for something trivial. */
	/* Returns 0 if equal, 1 if they differ in length, and 2 if their content differs at some point, -1 for erronous length on a string causing an error.*/
	static BOOL FrlStringCmp(FERALSTRING* String1, FERALSTRING* String2)
	{
		if (String1->Length != String2->Length)
		{
			/* If they differ in length, there is no way they can possibly be equal. */
			return 1;
		}

		/* Check for every single letter. If it's not the same, well, they're not equal. */
		for (UINT64 i = 0; i < String1->Length; i++)
		{
			if (String1->Content[i] == '\0' || String2->Content[i] == '\0')
			{
				return -1;
			}

			if (String1->Content[i] != String2->Content[i])
			{
				return 2;
			}
		}
		return 0;
	}

	/* Create a string. */
	FERALSTATUS FrlCreateString(IN FERALSTRING* StringLocation, UINT64 Length, WSTRING Content);

	/* Delete a string. */
	FERALSTATUS FrlDeleteString(IN FERALSTRING* String);

	/* Clone a string */
	FERALSTATUS FrlCloneString(IN FERALSTRING* Source, IN FERALSTRING* OutLocation);

#if defined(__cplusplus)
}
#endif

#endif
