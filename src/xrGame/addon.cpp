#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "addon_owner.h"

CAddon::CAddon()
{
	m_local_transform.identity			();
}

void CAddon::Load(LPCSTR section)
{
	inherited::Load						(section);

	m_SlotType							= pSettings->r_string(section, "slot_type");
	m_IconOffset						= pSettings->r_fvector2(section, "icon_offset");
	m_MotionsSuffix						= pSettings->r_string(section, "motions_suffix");
	if (pSettings->line_exist(section, "addon_type"))
	{
		shared_str addon_type			= pSettings->r_string(section, "addon_type");
		if (addon_type == "magazine")
			AddModule<CMagazine>		();
		else if (addon_type == "scope")
			AddModule<CScope>			(cNameSect());
		else if (addon_type == "silencer")
			AddModule<CSilencer>		(cNameSect());
		else if (addon_type == "grenade_launcher")
			AddModule<CGrenadeLauncher>	(cNameSect());
	}
}

void CAddon::RenderHud()
{
	::Render->set_Transform				(&m_hud_transform);
	::Render->add_Visual				(Visual());
}

void CAddon::RenderWorld(Fmatrix CR$ parent_trans)
{
	Fmatrix								trans;
	trans.mul							(parent_trans, m_local_transform);
	::Render->set_Transform				(&trans);
	::Render->add_Visual				(Visual());
}

void CAddon::updateLocalTransform(Fmatrix CPC parent_trans)
{
	if (parent_trans)
		m_local_transform				= *parent_trans;
	else
		m_local_transform.identity		();
	CAddonOwner* ao						= Cast<CAddonOwner*>();
	if (ao)
	{
		for (auto s : ao->AddonSlots())
		{
			if (s->addon && !s->forwarded_slot)
				s->updateAddonLocalTransform();
		}
	}
}

void CAddon::updateHudTransform(Fmatrix CR$ parent_trans)
{
	m_hud_transform.mul					(parent_trans, m_local_transform);
}
