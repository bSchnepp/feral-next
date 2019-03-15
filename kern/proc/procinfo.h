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
 
 #include <feral/stdtypes.h>
 
typedef enum SdPriorityLevel
{
	SCHED_PRIORITY_LEVEL_IDLE = 0x0,
	SCHED_PRIORITY_LEVEL_FAR_BELOW_NORMAL = 0x8000,
	SCHED_PRIORITY_LEVEL_BELOW_NORMAL = 0x8001,
	SCHED_PRIORITY_LEVEL_NORMAL = 0x0001,
	SCHED_PRIORITY_LEVEL_ABOVE_NORMAL = 0x0001,
	SCHED_PRIORITY_LEVEL_FAR_ABOVE_NORMAL = 0x0002,
	
	SCHED_PRIORITY_LEVEL_REALTIME = 0x7FFF,
}SdPriorityLevel;
 
/*Processor control region (various bits of info about this processor) */
typedef struct PcProcessor
{
#if defined(__i386__) || defined(__x86_64__)
	const char ProcessorVendor[12];
	const char ProcessorName[48];
#elif defined(__arm__) || defined(__aarch64__)
	const char *ProcessorVendor;
	const char *ProcessorName;
	const char *Architecture;
	const char *PartNumber;
	const char *PartRevision;
#endif

	UINT64	ProcNumber;
	UINT32	SMTThreads;
	
	//TODO
}PcProcessor;