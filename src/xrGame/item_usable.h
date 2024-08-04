#pragma once
#include "module.h"

struct SAction
{
	shared_str							title									= 0;
	shared_str							query_functor							= 0;
	shared_str							action_functor							= 0;
	shared_str							use_functor								= 0;
	float								duration								= 0.f;
	u16									item_id									= u16_max;
};

class MUsable : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mUsable; }

public:
										MUsable									(CGameObject* obj);

private:
	xr_vector<SAction>					m_actions								= {};

protected:
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	SAction*							getAction								(int num);
	bool								performAction							(int num, bool skip_query = false, u16 item_id = u16_max);
};
