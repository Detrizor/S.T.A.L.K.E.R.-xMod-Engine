#include "StdAfx.h"
#include ".\r__occlusion.h"

#include "QueryHelper.h"

R_occlusion::R_occlusion(void)
{
	enabled			= strstr(Core.Params,"-no_occq")?FALSE:TRUE;
}
R_occlusion::~R_occlusion(void)
{
	occq_destroy	();
}
void	R_occlusion::occq_create	(u32	limit	)
{
	pool.reserve	(limit);
	used.reserve	(limit);
	fids.reserve	(limit);
	for (u32 it=0; it<limit; it++)	{
		_Q	q;	q.order	= it;
		if	(FAILED( CreateQuery(&q.Q, D3DQUERYTYPE_OCCLUSION) ))	break;
		pool.push_back	(q);
	}
	std::reverse	(pool.begin(), pool.end());
}
void	R_occlusion::occq_destroy	(				)
{
	while	(!used.empty())	{
		_RELEASE(used.back().Q);
		used.pop_back	();
	}
	while	(!pool.empty())	{
		_RELEASE(pool.back().Q);
		pool.pop_back	();
	}
	used.clear	();
	pool.clear	();
	fids.clear	();
}
u32		R_occlusion::occq_begin		(u32&	ID		)
{
	if (!enabled)		return 0;

	//	Igor: prevent release crash if we issue too many queries
	if (pool.empty())
	{
		static float prev_time = Device.GetTimerGlobal()->GetElapsed_sec();
		float cur_time = Device.GetTimerGlobal()->GetElapsed_sec();
		if (cur_time - prev_time > 10.f)
		{
			Msg(" RENDER [Warning]: Too many occlusion queries were issued(>1536)!!!");
			prev_time = cur_time;
		}
		ID = iInvalidHandle;
		return 0;
	}

	RImplementation.stats.o_queries	++;
	if (!fids.empty())	{
		ID				= fids.back	();	
		fids.pop_back	();
		VERIFY				( pool.size() );
		used[ID]			= pool.back	();
	} else {
		ID					= used.size	();
		VERIFY				( pool.size() );
		used.push_back		(pool.back());
	}
	pool.pop_back			();
	//CHK_DX					(used[ID].Q->Issue	(D3DISSUE_BEGIN));
	CHK_DX					(BeginQuery(used[ID].Q));
	
	// Msg				("begin: [%2d] - %d", used[ID].order, ID);

	return			used[ID].order;
}
void	R_occlusion::occq_end		(u32&	ID		)
{
	if (!enabled)		return;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle) return;

	// Msg				("end  : [%2d] - %d", used[ID].order, ID);
	//CHK_DX			(used[ID].Q->Issue	(D3DISSUE_END));
	CHK_DX			(EndQuery(used[ID].Q));
}

R_occlusion::occq_result R_occlusion::occq_get(u32& ID)
{
	if (!enabled)
		return 0xffffffff;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle)
		return 0xFFFFFFFF;

	occq_result	fragments = 0;
	Device.Statistic->RenderDUMP_Wait.Begin();
	VERIFY2(ID < used.size(), make_string("_Pos = %d, size() = %d ", ID, used.size()));

	do
	{
		HRESULT hr = GetData(used[ID].Q, &fragments, sizeof(fragments));
		if (hr == S_OK)
			break;
		else if (hr == S_FALSE)
		{
			fragments = (occq_result)-1; //0xffffffff;
			break;
		}
		else if (hr == D3DERR_DEVICELOST)
		{
			fragments = 0xffffffff;
			break;
		}
	} while (true);

	HRESULT hr;
	Device.Statistic->RenderDUMP_Wait.End();
	if (hr == D3DERR_DEVICELOST)	fragments = 0xffffffff;

	if (0 == fragments)	RImplementation.stats.o_culled++;

	// insert into pool (sorting in decreasing order)
	_Q& Q = used[ID];
	if (pool.empty())
		pool.push_back(Q);
	else
	{
		int		it = int(pool.size()) - 1;
		while ((it >= 0) && (pool[it].order < Q.order))
			it--;
		pool.insert(pool.begin() + it + 1, Q);
	}

	// remove from used and shrink as nesessary
	used[ID].Q = 0;
	fids.push_back(ID);
	ID = 0;
	return fragments;
}
