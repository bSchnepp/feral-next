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

// Include stuff belonging to the platform.
#include <feral/stdtypes.h>
#include <waypoint.h>

#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
VOID WpFunctionCallback(WpDebugReportCallbackFlags flags, WpDebugReportComType objType, uint64_t hObj, int32_t code, const char* ssName, const char* message, void* userData)
{
	// WP_DEBUG_INFORMATION_BIT
	// WP_DEBUG_WARNING_BIT
	// WP_DEBUG_PERFORMANCE_BIT
	// WP_DEBUG_ERROR_BIT
	// WP_DEBUG_GENERAL_BIT

	if (flags & WP_ERROR_BIT)
	{
		WpConOutColor(SYSERR, message, "0xFF0000");
	}
	else
	{
		WpConOut(SYSERR, message);
	}

}
#endif

UINT32 WayMain(IN UINT32 ArgumentCount, IN WSTRING* Arguments, INOPT HTASK Parent, INOPT HUSER User, INOPT HANDLE Directory)
{
	ContentLaguage lang;
	WpGetSystemLanguage(&lang);

	if (strcmp("en_US", lang.LangShort == 0)
	{
		MessageBox(NULL, L"Testing!", L"Hello, world!", MB_OK, &lang);
	}
	return 0;
}


#if 0
	lang->LangShort = "en_US";
	lang->LangLong = L"English - US";
	lang->IsRightToLeft = 0;
#endif
