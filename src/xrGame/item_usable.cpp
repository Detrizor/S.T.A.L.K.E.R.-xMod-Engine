#include "stdafx.h"
#include "item_usable.h"
#include "GameObject.h"
#include "addon.h"
#include "string_table.h"
#include "script_engine.h"
#include "script_game_object.h"

SAction::SAction(CGameObject& _obj, u8 _num, bool manage) :
	obj(_obj),
	num(_num),
	manage_action(manage)
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
		auto& act						= m_actions.emplace_back(O, i);
		act->title						= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_query_functor", i);
		act->query_functor				= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_action_functor", i);
		act->action_functor				= pSettings->r_string(O.cNameSect(), *line);
		
		line.printf						("use%d_functor", i);
		act->use_functor					= READ_IF_EXISTS(pSettings, r_string, O.cNameSect(), *line, 0);
		
		line.printf						("use%d_duration", i);
		act->duration					= READ_IF_EXISTS(pSettings, r_float, O.cNameSect(), *line, 0.f);
	}
}

static bool check_last_addon(MAddon* addon, VSlots CR$ slots, int& nesting_level)
{
	for (auto it = slots.end(); it != slots.begin();)
	{
		auto& slot						= *--it;
		if (!slot->addons.empty())
		{
			auto back_addon				= slot->addons.back();
			if (back_addon == addon)
				return					true;
			else
				return					(back_addon->slots) ? check_last_addon(addon, *back_addon->slots, ++nesting_level) : false;
		}
	}
	return								false;
}

void MUsable::emplace_manage_action(MAddon* addon, int nesting_level)
{
	int num								= m_actions.size() + 1;
	auto& act							= m_actions.emplace_back(O, num, true);
	act->title							= ">";
	while (nesting_level--)
		act->title.printf				("%s>", act->title.c_str());
	act->title.printf					("%s %s", act->title.c_str(), addon->I->getNameShort());
	act->query_functor					= pSettings->r_string(O.cNameSect(), "manage_addon_query_functor");
	act->action_functor					= pSettings->r_string(O.cNameSect(), "manage_addon_action_functor");
	act->item_id						= addon->O.ID();
}

void MUsable::fill_manage_actions(VSlots CR$ slots, MAddon* ignore, int nesting_level)
{
	for (auto& slot : slots)
	{
		for (auto addon : slot->addons)
		{
			if (addon != ignore)
			{
				emplace_manage_action	(addon, nesting_level);
				if (addon->slots)
					fill_manage_actions	(*addon->slots, ignore, nesting_level + 1);
			}
		}
	}
}

float MUsable::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnAddon:
	{
		auto addon						= static_cast<MAddon*>(data);
		int nesting_level				= 0;
		if (check_last_addon(addon, O.getModule<MAddonOwner>()->AddonSlots(), nesting_level))
		{
			if (param > 0)
				emplace_manage_action	(addon, nesting_level);
			else
				m_actions.erase_data_if	([addon](xptr<SAction> CR$ action) { return (action->item_id == addon->O.ID()); });
		}
		else if (param == 0 || param == 1)
		{
			for (auto it = m_actions.begin(), it_e = m_actions.end(); it != it_e; ++it)
			{
				if ((*it)->manage_action)
				{
					m_actions.erase		(it, it_e);
					break;
				}
			}
			fill_manage_actions			(O.getModule<MAddonOwner>()->AddonSlots(), (param > 0) ? nullptr : addon, 0);
		}
		break;
	}
	}

	return								CModule::aboba(type, data, param);
}

SAction* MUsable::getAction(int num)
{
	return								(num > 0 && num <= m_actions.size()) ? m_actions[num - 1].get() : nullptr;
}

SAction* MUsable::getAction(shared_str CR$ title)
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
