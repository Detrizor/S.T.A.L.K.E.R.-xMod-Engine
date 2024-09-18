#include "stdafx.h"
#include "addon.h"
#include "magazine.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "item_usable.h"
#include "addon_owner.h"
#include "WeaponMagazined.h"

void MAddon::addAddonModules(CGameObject& O, shared_str CR$ addon_sect)
{
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "magazine", FALSE))
		O.addModule<MMagazine>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "scope", FALSE))
		O.addModule<MScope>				(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "muzzle", FALSE))
		O.addModule<MMuzzle>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "silencer", FALSE))
		O.addModule<CSilencer>			(addon_sect);
	if (READ_IF_EXISTS(pSettings, r_bool, addon_sect, "grenade_launcher", FALSE))
		O.addModule<MGrenadeLauncher>	(addon_sect);
}

MAddon::MAddon(CGameObject* obj, LPCSTR section) : CModule(obj), m_section(section)
{
	m_SlotType							= pSettings->r_string(m_section, "slot_type");
	m_icon_origin						= pSettings->r_fvector2(m_section, "inv_icon_origin");
	m_low_profile						= pSettings->r_bool(m_section, "low_profile");
	m_front_positioning					= pSettings->r_bool(m_section, "front_positioning");
	
	m_mount_length						= pSettings->r_float(m_section, "mount_length");
	m_profile_length					= pSettings->r_fvector2(m_section, "profile_length");

	if (obj)
	{
		addAddonModules					(O, m_section);
		if (auto ao = O.getModule<MAddonOwner>())
			slots						= const_cast<VSlots*>(&ao->AddonSlots());
	}
}

static MAddonOwner* get_root_addon_owner(MAddonOwner* ao)
{
	if (ao->O.H_Parent())
		if (auto ao_parent_ao = ao->O.H_Parent()->getModule<MAddonOwner>())
			return						get_root_addon_owner(ao_parent_ao);
	return								ao;
}

void MAddon::RenderHud() const
{
	if (Device.SVP.isRendering())
		if (auto scope = O.getModule<MScope>())
			if (auto wpn = get_root_addon_owner(O.H_Parent()->getModule<MAddonOwner>())->O.scast<CWeaponMagazined*>())
				if (wpn->getActiveScope() == scope)
					return;

	::Render->set_Transform				(m_hud_transform);
	::Render->add_Visual				(O.Visual());
}

void MAddon::RenderWorld(Fmatrix CR$ parent_trans) const
{
	::Render->set_Transform				(static_cast<Dmatrix>(parent_trans).mulB_43(m_local_transform));
	::Render->add_Visual				(O.Visual());
}

Fvector2 MAddon::getIconOrigin(u8 type) const
{
	if (type)
	{
		shared_str						tmp;
		tmp.printf						("inv_icon_origin_%d", type);
		if (pSettings->line_exist(m_section, tmp.c_str()))
			return						pSettings->r_fvector2(m_section, tmp.c_str());
	}
	return								m_icon_origin;
}

void MAddon::updateHudTransform(Dmatrix CR$ parent_trans)
{
	m_hud_transform.mul					(parent_trans, m_local_transform);
}

int MAddon::getLength(float step, eLengthType type) const
{
	float len							= 0.f;
	switch (type)
	{
	case Mount: len						= m_mount_length; break;
	case ProfileFwd: len				= m_profile_length.x; break;
	case ProfileBwd: len				= m_profile_length.y; break;
	}
	return								static_cast<int>(ceil(len / step));
}

float MAddon::aboba(EEventTypes type, void* data, int param)
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

void MAddon::attach(CAddonSlot CPC slot)
{
	setSlotIdx							(slot->idx);
	if (auto usable = O.getModule<MUsable>())
		if (auto act = usable->getAction("st_attach"))
			usable->performAction		(act->num, true, slot->parent_ao->O.ID());
}

bool MAddon::tryAttach(MAddonOwner CPC ao, u16 slot_idx)
{
	if (slot_idx != u16_max)
	{
		auto& slot						= ao->AddonSlots()[slot_idx];
		if (slot->CanTake(this))
		{
			attach						(slot.get());
			return						true;
		}
	}

	if (auto slot = ao->findAvailableSlot(this))
	{
		attach							(slot);
		return							true;
	}

	return								false;
}
