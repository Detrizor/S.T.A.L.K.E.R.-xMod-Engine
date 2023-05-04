#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"

CAddon::CAddon()
{
	m_SlotType							= 0;
	m_IconOffset						= { 0.f, 0.f };
}

void CAddon::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_SlotType							= pSettings->r_string(section, "slot_type");
	m_IconOffset						= pSettings->r_fvector2(section, "icon_offset");
	if (pSettings->line_exist(section, "addon_type"))
	{
		shared_str addon_type			= pSettings->r_string(section, "addon_type");
		if (addon_type == "magazine")
			AddModule<CMagazine>		();
		else if (addon_type == "scope")
		{
			AddModule<CScope>			(cNameSect());
			m_hud_offset[0]				= pSettings->r_fvector3(section, "hud_offset_pos");
			m_hud_offset[1]				= pSettings->r_fvector3(section, "hud_offset_rot");
		}
		else if (addon_type == "silencer")
			AddModule<CSilencer>		(cNameSect());
		else if (addon_type == "grenade_launcher")
			AddModule<CGrenadeLauncher>	(cNameSect());
	}
}

void CAddon::Render(Fmatrix* pos)
{
	::Render->set_Transform				(pos);
	::Render->add_Visual				(Visual());
}
