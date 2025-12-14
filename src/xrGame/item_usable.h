#pragma once
#include "module.h"
#include "script_game_object.h"

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
	float								duration								= 0.f;
	u16									item_id									= u16_max;
	
	shared_str							query_functor_str;
	shared_str							action_functor_str;
	shared_str							use_functor_str							= 0;

	bool								performQueryFunctor						()		{ return perform(*query_functor); }
	bool								performActionFunctor					()		{ return perform(*action_functor); }

	bool								perform									(::luabind::functor<bool>& functor);

	xptr<::luabind::functor<bool>> query_functor{};
	xptr<::luabind::functor<bool>> action_functor{};
	xptr<::luabind::functor<bool>> use_functor{ nullptr };
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
	void								sOnAddon							O$	(MAddon* addon, int attach_type);

public:
	auto CR$							getActions								()		{ return m_actions; }

	SAction*							getAction								(int num);
	SAction*							getAction								(shared_str CR$ title);
	bool								performAction							(int num, bool skip_query = false, u16 item_id = u16_max);
};
