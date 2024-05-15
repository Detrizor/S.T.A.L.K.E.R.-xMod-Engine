#include "stdafx.h"
#include "item_usable.h"
#include "GameObject.h"
#include "addon.h"
#include "string_table.h"

CUsable::CUsable(CGameObject* obj) : CModule(obj)
{
	int i								= 0;
	shared_str							line;
	while (line.printf("use%d_title", ++i), pSettings->line_exist(O.cNameSect(), line))
	{
		SAction							act;
		act.title						= pSettings->r_string(*O.cNameSect(), *line);
		
		line.printf						("use%d_query_functor", i);
		act.query_functor				= pSettings->r_string(*O.cNameSect(), *line);
		
		line.printf						("use%d_action_functor", i);
		act.action_functor				= pSettings->r_string(*O.cNameSect(), *line);
		
		line.printf						("use%d_functor", i);
		act.use_functor					= READ_IF_EXISTS(pSettings, r_string, *O.cNameSect(), *line, 0);
		
		line.printf						("use%d_duration", i);
		act.duration					= READ_IF_EXISTS(pSettings, r_float, *O.cNameSect(), *line, 0.f);
		
		m_actions.push_back				(act);
	}
}

float CUsable::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnAddon:
	{
		auto addon						= (CAddon*)data;
		if (param)
		{
			SAction						act;
			act.title.printf			("%s %s", *CStringTable().translate("st_manage"), addon->cast<CInventoryItem*>()->NameShort());
			act.query_functor			= pSettings->r_string(*O.cNameSect(), "manage_addon_query_functor");
			act.action_functor			= pSettings->r_string(*O.cNameSect(), "manage_addon_action_functor");
			act.item_id					= addon->O.ID();
			m_actions.push_back			(act);
		}
		else
		{
			for (auto it = m_actions.begin(), it_e = m_actions.end(); it != it_e; it++)
			{
				if ((*it).item_id == addon->O.ID())
				{
					m_actions.erase		(it);
					break;
				}
			}
		}
		break;
	}
	}

	return								CModule::aboba(type, data, param);
}

SAction CP$ CUsable::getAction(int num) const
{
	return								(num > 0 && num <= m_actions.size()) ? &m_actions[num - 1] : nullptr;
}
