#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"

CAddon::CAddon(CGameObject* obj) : CModule(obj)
{
	m_SlotType							= pSettings->r_string(O.cNameSect(), "slot_type");
	m_IconOffset						= pSettings->r_fvector2(O.cNameSect(), "icon_offset");
	m_low_profile						= pSettings->r_bool(O.cNameSect(), "low_profile");
	m_anm_prefix						= pSettings->r_string(O.cNameSect(), "anm_prefix");
	m_front_positioning					= pSettings->r_bool(O.cNameSect(), "front_positioning");
	if (pSettings->line_exist(O.cNameSect(), "addon_type"))
	{
		shared_str addon_type			= pSettings->r_string(O.cNameSect(), "addon_type");
		if (addon_type == "magazine")
			O.AddModule<CMagazine>		();
		else if (addon_type == "scope")
			O.AddModule<CScope>			(O.cNameSect());
		else if (addon_type == "silencer")
			O.AddModule<CSilencer>		(O.cNameSect());
		else if (addon_type == "grenade_launcher")
			O.AddModule<CGrenadeLauncher>	(O.cNameSect());
	}
	
	m_mount_length						= pSettings->r_float(O.cNameSect(), "mount_length");
	m_profile_length					= pSettings->r_fvector2(O.cNameSect(), "profile_length");
}

void CAddon::RenderHud() const
{
	::Render->set_Transform				(m_hud_transform);
	::Render->add_Visual				(O.Visual());
}

void CAddon::RenderWorld(Fmatrix CR$ trans) const
{
	::Render->set_Transform				(Fmatrix().mul(trans, m_local_transform));
	::Render->add_Visual				(O.Visual());
}

void CAddon::updateLocalTransform(Fmatrix CR$ parent_trans)
{
	m_local_transform					= parent_trans;
}

void CAddon::updateHudTransform(Fmatrix CR$ parent_trans)
{
	m_hud_transform.mul					(parent_trans, m_local_transform);
}

int CAddon::getLength(float step, eLengthType type) const
{
	float len							= 0.f;
	switch (type)
	{
	case Mount: len						= m_mount_length; break;
	case ProfileFwd: len				= m_profile_length.x; break;
	case ProfileBwd: len				= m_profile_length.y; break;
	}
	return								(int)ceil(len / step);
}

float CAddon::aboba(EEventTypes type, void* data, int param)
{
	if (type == eSyncData)
	{
		auto se_obj						= (CSE_ALifeDynamicObject*)data;
		auto m							= se_obj->getModule<CSE_ALifeModuleAddon>(!!param);
		if (param)
		{
			m->m_slot_idx				= m_slot_idx;
			m->m_slot_pos				= m_slot_pos;
		}
		else if (m)
		{
			m_slot_idx					= m->m_slot_idx;
			m_slot_pos					= m->m_slot_pos;
		}
	}
	
	return								CModule::aboba(type, data, param);
}
