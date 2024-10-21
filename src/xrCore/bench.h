#pragma once

class CRenderDevice;

class XRCORE_API xBench
{
	struct bench_data
	{
		u32								cnt										= 0;
		float							sum_time								= 0.f;
		u32								frames									= 0;
		u32								last_frame								= 0;
	};

public:
										xBench									(LPCSTR tag_, bool aggregate_ = true);
										~xBench									();

private:
	static xr_umap<_STD string, bench_data> statistics;
	static CRenderDevice*				device;
	const _STD string					tag;
	const bool							aggregate;

	float								start_time								= 0.f;

public:
	static void							setupDevice								(CRenderDevice* d)		{ device = d; }
	static void							flushStatistics							();
};

#define BENCH xBench bench(__FUNCTION__)
#define BENCH_TAG(tag) xBench bench(tag)

#define IBENCH xBench bench(__FUNCTION__, false)
#define IBENCH_TAG(tag) xBench bench(tag, false)

#define BENCH_FUN(func) { xBench b(#func); func; }
#define BENCH_FUN_TAG(func, tag) { xBench b(tag); func; }
#define BENCH_FUN_RET(func) [&]{ xBench b(#func); auto res = func; return res; }()
#define BENCH_FUN_RET_TAG(func, tag) [&]{ xBench b(tag); auto res = func; return res; }()

#define IBENCH_FUN(func) { xBench b(#func, false); func; }
#define IBENCH_FUN_TAG(func, tag) { xBench b(tag, false); func; }
#define IBENCH_FUN_RET(func) [&]{ xBench b(#func, false); auto res = func; return res; }()
#define IBENCH_FUN_RET_TAG(func, tag) [&]{ xBench b(tag, false); auto res = func; return res; }()
