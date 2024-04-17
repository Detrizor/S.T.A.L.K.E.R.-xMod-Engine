#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "addon_owner.h"

void CAddon::Load(LPCSTR section)
{
	inherited::Load						(section);

	m_length							= pSettings->r_float(section, "length");
	m_SlotType							= pSettings->r_string(section, "slot_type");
	m_IconOffset						= pSettings->r_fvector2(section, "icon_offset");
	m_low_profile						= pSettings->r_bool(section, "low_profile");
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

void CAddon::RenderHud() const
{
	::Render->set_Transform				(m_hud_transform);
	::Render->add_Visual				(Visual());

	if (auto ao = Cast<CAddonOwner CP$>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				a->RenderHud			();
}

void CAddon::RenderWorld(Fmatrix CR$ trans) const
{
	::Render->set_Transform				(Fmatrix().mul(trans, m_local_transform));
	::Render->add_Visual				(Visual());
	
	if (auto ao = Cast<CAddonOwner CP$>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				a->RenderWorld			(trans);
}

void CAddon::updateLocalTransform(Fmatrix CR$ parent_trans)
{
	m_local_transform					= parent_trans;

	if (auto ao = Cast<CAddonOwner*>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				s->updateAddonLocalTransform(a);
}

void CAddon::updateHudTransform(Fmatrix CR$ parent_trans)
{
	m_hud_transform.mul					(parent_trans, m_local_transform);
	
	if (auto ao = Cast<CAddonOwner*>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				a->updateHudTransform	(parent_trans);
}
