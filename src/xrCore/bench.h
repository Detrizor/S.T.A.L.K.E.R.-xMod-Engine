#pragma once

class CRenderDevice;

class XRCORE_API xBench
{
protected:
										xBench									(bool init);

protected:
	static CRenderDevice*				device;
	const float							m_start_time;

public:
	static void							setupDevice								(CRenderDevice* d);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//aggregate benchmark - flushes results on application exit
class XRCORE_API aBench : public xBench
{
	struct measures
	{
										measures								(LPCSTR tag_) : tag(tag_) {}

		const shared_str				tag;

		float							sum_time								= 0.f;
		u32								cnt										= 0;
		u32								frames									= 0;
		u32								last_frame								= 0;
	};

public:
										aBench									(measures* data);
										~aBench();

private:
	static xr_vector<xptr<measures>>	statistics;
	measures* const						m_data;

public:
	static measures*					createMeasures							(LPCSTR tag);
	static void							flushStatistics							();
};

#define BENCH static auto d = aBench::createMeasures(__FUNCTION__); aBench bench(d)
#define BENCH_TAG(tag) static auto d = aBench::createMeasures(tag); aBench bench(d)

#define BENCH_FUN(func) { static auto d = aBench::createMeasures(#func); aBench b(d); func; }
#define BENCH_FUN_TAG(func, tag) { static auto d = aBench::createMeasures(tag); aBench b(d); func; }
#define BENCH_FUN_RET(func) [&]{ static auto d = aBench::createMeasures(#func); aBench b(d); auto res = func; return res; }()
#define BENCH_FUN_RET_TAG(func, tag) [&]{ static auto d = aBench::createMeasures(tag); aBench b(d); auto res = func; return res; }()

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// instant benchmark - flashes result straight on destroy
class XRCORE_API iBench : public xBench
{
public:
										iBench									(LPCSTR tag);
										~iBench									();

private:
	const shared_str					m_tag;
};

#define IBENCH iBench bench(__FUNCTION__)
#define IBENCH_TAG(tag) iBench bench(tag)

#define IBENCH_FUN(func) { iBench b(#func); func; }
#define IBENCH_FUN_TAG(func, tag) { iBench b(tag); func; }
#define IBENCH_FUN_RET(func) [&]{ iBench b(#func); auto res = func; return res; }()
#define IBENCH_FUN_RET_TAG(func, tag) [&]{ iBench b(tag); auto res = func; return res; }()
