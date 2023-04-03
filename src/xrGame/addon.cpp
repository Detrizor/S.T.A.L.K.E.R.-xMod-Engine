#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"

CAddon::CAddon()
{
	m_SlotType							= 0;
}

void CAddon::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_SlotType							= pSettings->r_string(section, "slot_type");
	if (pSettings->line_exist(section, "addon_type"))
	{
		shared_str addon_type			= pSettings->r_string(section, "addon_type");
		if (addon_type == "magazine")
			AddModule<CMagazine>		();
		else if (addon_type == "scope")
			AddModule<CScope>			();
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
