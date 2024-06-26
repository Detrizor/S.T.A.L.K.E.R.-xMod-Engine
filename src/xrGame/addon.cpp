#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "item_usable.h"
#include "addon_owner.h"
#include "WeaponMagazined.h"
#include "barrel.h"

void CAddon::addAddonModules(CGameObject& O, shared_str CR$ addon_sect)
{
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "magazine", FALSE))
		O.AddModule<CMagazine>			();
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "scope", FALSE))
		O.AddModule<CScope>				(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "barrel", FALSE))
		O.AddModule<CBarrel>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "muzzle", FALSE))
		O.AddModule<CMuzzle>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "silencer", FALSE))
		O.AddModule<CSilencer>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "grenade_launcher", FALSE))
		O.AddModule<CGrenadeLauncher>	(addon_sect);
}

CAddon::CAddon(CGameObject* obj) : CModule(obj)
{
	m_SlotType							= pSettings->r_string(O.cNameSect(), "slot_type");
	m_icon_origin						= pSettings->r_fvector2(O.cNameSect(), "inv_icon_origin");
	m_low_profile						= pSettings->r_bool(O.cNameSect(), "low_profile");
	m_front_positioning					= pSettings->r_bool(O.cNameSect(), "front_positioning");
	
	m_mount_length						= pSettings->r_float(O.cNameSect(), "mount_length");
	m_profile_length					= pSettings->r_fvector2(O.cNameSect(), "profile_length");

	addAddonModules						(O, O.cNameSect());
}

CAddonOwner* get_root_addon_owner(CAddonOwner* ao)
{
	if (ao->O.H_Parent())
		if (auto ao_parent_ao = ao->O.H_Parent()->Cast<CAddonOwner*>())
			return						get_root_addon_owner(ao_parent_ao);
	return								ao;
}

void CAddon::RenderHud() const
{
	if (Device.SVP.isRendering())
		if (auto scope = cast<CScope*>())
			if (auto wpn = get_root_addon_owner(O.H_Parent()->Cast<CAddonOwner*>())->cast<CWeaponMagazined*>())
				if (wpn->getActiveScope() == scope)
					return;

	::Render->set_Transform				(m_hud_transform);
	::Render->add_Visual				(O.Visual());
}

void CAddon::RenderWorld(Fmatrix CR$ parent_trans) const
{
	::Render->set_Transform				(static_cast<Dmatrix>(parent_trans).mulB_43(m_local_transform));
	::Render->add_Visual				(O.Visual());
}

Fvector2 CAddon::getIconOrigin(u8 type) const
{
	if (type)
	{
		shared_str						tmp;
		tmp.printf						("inv_icon_origin_%d", type);
		if (pSettings->line_exist(O.cNameSect(), *tmp))
			return						pSettings->r_fvector2(O.cNameSect(), *tmp);
	}
	return								m_icon_origin;
}

void CAddon::updateHudTransform(Dmatrix CR$ parent_trans)
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

void CAddon::attach(CAddonOwner CPC ao, u16 slot_idx)
{
	setSlotIdx							(slot_idx);
	cast<CUsable*>()->performAction		(2, true, ao->O.ID());
}

bool CAddon::tryAttach(CAddonOwner CPC ao, u16 slot_idx)
{
	if (slot_idx != u16_max)
	{
		if (ao->AddonSlots()[slot_idx]->CanTake(this))
		{
			attach						(ao, slot_idx);
			return						true;
		}
	}

	if (auto slot = ao->findAvailableSlot(this))
	{
		attach							(ao, slot->idx);
		return							true;
	}

	return								false;
}
