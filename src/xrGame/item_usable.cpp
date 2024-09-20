#include "stdafx.h"
#include "item_usable.h"
#include "GameObject.h"
#include "addon.h"
#include "string_table.h"
#include "script_engine.h"
#include "script_game_object.h"

SAction::SAction(CGameObject& _obj, u8 _num) : obj(_obj), num(_num)
{
}

bool SAction::perform(shared_str CR$ functor)
{
	::luabind::functor<bool>			funct;
	if (ai().script_engine().functor(functor.c_str(), funct))
		return							funct(obj.lua_game_object(), num);
	return								false;
}

MUsable::MUsable(CGameObject* obj) : CModule(obj)
{
	int i								= 0;
	shared_str							line;
	while (line.printf("use%d_title", ++i), pSettings->line_exist(O.cNameSect(), line))
	{
		SAction& act					= *m_actions.emplace_back(O, i);
		act.title						= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_query_functor", i);
		act.query_functor				= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_action_functor", i);
		act.action_functor				= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_functor", i);
		act.use_functor					= READ_IF_EXISTS(pSettings, r_string, O.cNameSect(), *line, 0);
		
		line.printf						("use%d_duration", i);
		act.duration					= READ_IF_EXISTS(pSettings, r_float, O.cNameSect(), *line, 0.f);
	}
}

float MUsable::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnAddon:
	{
		auto addon						= static_cast<MAddon*>(data);
		if (param)
		{
			int num						= m_actions.size() + 1;
			SAction& act				= *m_actions.emplace_back(O, num);
			act.title.printf			("%s %s", *CStringTable().translate("st_manage"), addon->I->getNameShort());
			act.query_functor			= pSettings->r_string(O.cNameSect(), "manage_addon_query_functor");
			act.action_functor			= pSettings->r_string(O.cNameSect(), "manage_addon_action_functor");
			act.item_id					= addon->O.ID();
		}
		else
			m_actions.erase_data_if		([addon](xptr<SAction> CR$ action) { return (action->item_id == addon->O.ID()); });
		break;
	}
	}

	return								CModule::aboba(type, data, param);
}

SAction* MUsable::getAction(int num)
{
	return								(num > 0 && num <= m_actions.size()) ? m_actions[num - 1].get() : nullptr;
}

SAction* MUsable::getAction(LPCSTR title)
{
	auto it								= m_actions.find_if([title](xptr<SAction> CR$ action) { return (action->title == title); });
	return								(it != m_actions.end()) ? it->get() : nullptr;
}

bool MUsable::performAction(int num, bool skip_query, u16 item_id)
{
	auto& action						= m_actions[num - 1];
	if (item_id != u16_max)
		action->item_id					= item_id;
	if (skip_query || action->performQuery())
	{
		action->performAction			();
		return							true;
	}
	return								false;
}
