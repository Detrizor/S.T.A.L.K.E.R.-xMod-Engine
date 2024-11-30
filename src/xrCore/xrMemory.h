#pragma once

#include "_types.h"

class XRCORE_API xrMemory final
{
	// Additional 16 bytes of memory almost like in original xr_aligned_offset_malloc
	// But for DEBUG we don't need this if we want to find memory problems
#ifdef DEBUG
	const size_t reserved = 0;
#else
	const size_t reserved = 16;
#endif

public:
	xrMemory() = default;
	void _initialize();
	void _destroy();

public:
	struct SProcessMemInfo
	{
		u64 PeakWorkingSetSize;
		u64 WorkingSetSize;
		u64 PagefileUsage;
		u64 PeakPagefileUsage;

		u64 TotalPhysicalMemory;
		s64 FreePhysicalMemory;
		u64 TotalVirtualMemory;
		u32 MemoryLoad;
	};

	void GetProcessMemInfo(SProcessMemInfo& minfo);
	size_t mem_usage();
	void mem_compact();

	IC void* mem_alloc(size_t size) { return malloc(size + reserved); };
	IC void* mem_realloc(void* ptr, size_t size) { return realloc(ptr, size + reserved); };
	IC void mem_free(void* ptr) { free(ptr); };
	IC void __stdcall mem_copy(void* dest, const void* src, u32 count) { memcpy(dest, src, count); }
	IC void __stdcall mem_fill(void* dest, int value, u32 count) { memset(dest, value, count); }

	IC void __stdcall mem_fill32(void* dest, u32 value, u32 count)
	{
		u32* ptr = static_cast<u32*>(dest);
		u32* end = ptr + count;
		while (ptr != end)
			*ptr++ = value;
	}
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(a, b) memset(a, 0, b)
#define CopyMemory(a, b, c) memcpy(a, b, c)
#define FillMemory(a, b, c) memset(a, c, b)

template <class T, typename... Args>
IC T* xr_new(Args&&... args)
{
	T* ptr = xr_alloc<T>(1);
	return new (ptr)T(_STD forward<Args>(args)...);
}

template <typename T>
IC void xr_delete(T&& ptr)
{
	if (ptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

// generic "C"-like allocations/deallocations
template <typename T>
IC T* xr_alloc(size_t count)
{
	return (T*)Memory.mem_alloc(count * sizeof(T));
}

template <typename T>
IC void xr_free(T*& ptr)
{
	if (ptr)
	{
		Memory.mem_free((void*)ptr);
		ptr = nullptr;
	}
};

IC void* xr_malloc(size_t size) { return Memory.mem_alloc(size); }
IC void* xr_realloc(void* ptr, size_t size) { return Memory.mem_realloc(ptr, size); }

XRCORE_API void log_vminfo();
