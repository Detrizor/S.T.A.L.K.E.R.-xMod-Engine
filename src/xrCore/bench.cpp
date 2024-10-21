#include "stdafx.h"
#include "bench.h"
#include "..\xrEngine\device.h"

xr_umap<_STD string, xBench::bench_data> xBench::statistics = {};
CRenderDevice* xBench::device = nullptr;

xBench::xBench(LPCSTR tag_, bool aggregate_) : tag(tag_), aggregate(aggregate_)
{
	start_time							= device->timeGlobal();
}

xBench::~xBench()
{
	float result						= device->timeGlobal() - start_time;
	if (aggregate)
	{
		auto& data						= statistics[tag];
		++data.cnt;
		data.sum_time					+= result;
		if (data.last_frame != device->dwFrame)
		{
			data.last_frame				= device->dwFrame;
			++data.frames;
		}
	}
	else
		Msg								("--benchmark [%s] result [%.3f]", tag.c_str(), result);
}

void xBench::flushStatistics()
{
	for (auto& bench : statistics)
	{
		float avg_time					= bench.second.sum_time / static_cast<float>(bench.second.cnt);
		float avg_per_frame				= bench.second.sum_time / static_cast<float>(bench.second.frames);
		Msg								("--benchmark [%s] sum [%.3f] cnt [%d] frames [%d] avg [%.3f] avg per frame [%.3f]",
			bench.first.c_str(),
			bench.second.sum_time,
			bench.second.cnt,
			bench.second.frames,
			avg_time,
			avg_per_frame
		);
	}
	statistics.clear					();
}
