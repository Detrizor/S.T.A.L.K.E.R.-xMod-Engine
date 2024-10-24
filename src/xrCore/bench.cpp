#include "stdafx.h"
#include "bench.h"
#include "../xrengine/device.h"

CRenderDevice* xBench::device = nullptr;

xBench::xBench(bool init) : m_start_time((init) ? device->GetTimerGlobal()->GetElapsed_sec() : -1.f)
{
}

void xBench::setupDevice(CRenderDevice* d)
{
	device = d;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

xr_vector<xptr<aBench::measures>> aBench::statistics = {};

aBench::aBench(measures* data) : xBench(device->isGameProcess()), m_data(data)
{
}

aBench::~aBench()
{
	if (m_start_time >= 0.f)
	{
		m_data->sum_time				+= device->GetTimerGlobal()->GetElapsed_sec() - m_start_time;
		++m_data->cnt;
		if (m_data->last_frame != device->dwFrame)
		{
			m_data->last_frame			= device->dwFrame;
			++m_data->frames;
		}
	}
}

aBench::measures* aBench::createMeasures(LPCSTR tag)
{
	return								statistics.emplace_back(tag).get();
}

void aBench::flushStatistics()
{
	for (auto& data : statistics)
	{
		float sum_time					= data->sum_time * 1000.f;
		float avg_time					= sum_time / static_cast<float>(data->cnt);
		float avg_per_frame				= sum_time / static_cast<float>(data->frames);
		Msg								("--benchmark [%s] sum [%.3f] cnt [%d] frames [%d] avg [%.3f] avg per frame [%.3f]",
			data->tag.c_str(),
			sum_time,
			data->cnt,
			data->frames,
			avg_time,
			avg_per_frame
		);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

iBench::iBench(LPCSTR tag) : xBench(true), m_tag(tag)
{
}

iBench::~iBench()
{
	float result						= device->GetTimerGlobal()->GetElapsed_sec() - m_start_time;
	Msg									("--benchmark [%s] result [%.3f]", m_tag.c_str(), result * 1000.f);
}
