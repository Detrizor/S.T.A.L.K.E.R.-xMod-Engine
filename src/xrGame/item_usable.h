#pragma once
#include "module.h"

class MAddon;
class CAddonSlot;

typedef xr_vector<xptr<CAddonSlot>> VSlots;

struct SAction
{
										SAction									(CGameObject& _obj, u8 _num, bool manage = false);

	CGameObject&						obj;
	const u8							num;
	const bool							manage_action;

	shared_str							title									= 0;
	shared_str							query_functor							= 0;
	shared_str							action_functor							= 0;
	shared_str							use_functor								= 0;
	float								duration								= 0.f;
	u16									item_id									= u16_max;

	bool								performQuery							()		{ return perform(query_functor); }
	bool								performAction							()		{ return perform(action_functor); }

	bool								perform									(shared_str CR$ functor);
};

class MUsable : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mUsable; }

public:
										MUsable									(CGameObject* obj);

private:
	xr_vector<xptr<SAction>>			m_actions								= {};

	void								emplace_manage_action					(MAddon* addon, int nesting_level);
	void								fill_manage_actions						(VSlots CR$ slots, MAddon* ignore, int nesting_level);

protected:
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	SAction*							getAction								(int num);
	SAction*							getAction								(shared_str CR$ title);
	bool								performAction							(int num, bool skip_query = false, u16 item_id = u16_max);
};
