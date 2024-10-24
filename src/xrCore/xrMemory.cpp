#include "stdafx.h"
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

xrMemory Memory;
// Also used in src\xrCore\xrDebug.cpp to prevent use of g_pStringContainer before it initialized
bool shared_str_initialized = false;

XRCORE_API bool g_allow_heap_min = true;

void xrMemory::_initialize()
{
	g_pStringContainer = new str_container();
	shared_str_initialized = true;
	g_pSharedMemoryContainer = new smem_container();
}

void xrMemory::_destroy()
{
	xr_delete(g_pSharedMemoryContainer);
	xr_delete(g_pStringContainer);
}

void xrMemory::GetProcessMemInfo(SProcessMemInfo& minfo)
{
	memset(&minfo, 0, sizeof(SProcessMemInfo));

	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);
	GlobalMemoryStatusEx(&mem);

	minfo.TotalPhysicalMemory = mem.ullTotalPhys;
	minfo.FreePhysicalMemory = mem.ullAvailPhys;
	minfo.TotalVirtualMemory = mem.ullTotalVirtual;
	minfo.MemoryLoad = mem.dwMemoryLoad;

	//--------------------------------------------------------------------
	PROCESS_MEMORY_COUNTERS pc;
	memset(&pc, 0, sizeof(PROCESS_MEMORY_COUNTERS));
	pc.cb = sizeof(pc);
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pc, sizeof(pc)))
	{
		minfo.PeakWorkingSetSize = pc.PeakWorkingSetSize;
		minfo.WorkingSetSize = pc.WorkingSetSize;
		minfo.PagefileUsage = pc.PagefileUsage;
		minfo.PeakPagefileUsage = pc.PeakPagefileUsage;
	}
}

XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
	MEMORY_BASIC_INFORMATION memory_info;
	memory_info.BaseAddress = 0;
	*_free = *reserved = *committed = 0;
	while (VirtualQuery(memory_info.BaseAddress, &memory_info, sizeof(memory_info)))
	{
		switch (memory_info.State)
		{
		case MEM_FREE: *_free += memory_info.RegionSize; break;
		case MEM_RESERVE: *reserved += memory_info.RegionSize; break;
		case MEM_COMMIT: *committed += memory_info.RegionSize; break;
		}
		memory_info.BaseAddress = (char*)memory_info.BaseAddress + memory_info.RegionSize;
	}
}

XRCORE_API void log_vminfo()
{
	size_t w_free, w_reserved, w_committed;
	vminfo(&w_free, &w_reserved, &w_committed);
#ifdef XR_X64
	Msg("* [x64 Engine]: free[%I64d MB], reserved[%u KB], committed[%u KB]", w_free / (1024 * 1024), w_reserved / 1024,
		w_committed / 1024);
#else
	Msg("* [x32 Engine]: free[%u K], reserved[%u K], committed[%u K]", w_free / 1024, w_reserved / 1024, w_committed / 1024);
#endif
}

size_t xrMemory::mem_usage()
{
	PROCESS_MEMORY_COUNTERS pmc = {};
	if (HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId()))
	{
		GetProcessMemoryInfo(h, &pmc, sizeof(pmc));
		CloseHandle(h);
	}
	return pmc.PagefileUsage;
}

void xrMemory::mem_compact()
{
	RegFlushKey(HKEY_CLASSES_ROOT);
	RegFlushKey(HKEY_CURRENT_USER);

	if (g_allow_heap_min)
		_heapmin();
	HeapCompact(GetProcessHeap(), 0);
	if (g_pStringContainer)
		g_pStringContainer->clean();
	if (g_pSharedMemoryContainer)
		g_pSharedMemoryContainer->clean();

	if (strstr(Core.Params, "-swap_on_compact"))
		SetProcessWorkingSetSize(GetCurrentProcess(), size_t(-1), size_t(-1));
}
