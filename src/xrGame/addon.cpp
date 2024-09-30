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
	m_attach_action						= pSettings->r_string(m_section, "attach_action");

	if (obj)
	{
		addAddonModules					(O, m_section);
		if (auto ao = O.getModule<MAddonOwner>())
			slots						= const_cast<VSlots*>(&ao->AddonSlots());
	}
}

SAction* MAddon::get_attach_action() const
{
	auto usable							= O.getModule<MUsable>();
	return								usable->getAction(m_attach_action);
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
			if (m_slot_status == attached)
			{
				m->m_slot_idx			= static_cast<s16>(m_slot_idx);
				m->m_slot_pos			= static_cast<s16>(m_slot_pos);
			}
			else
			{
				m->m_slot_idx			= s16_max;
				m->m_slot_pos			= s16_max;
			}
		}
		else if (m)
		{
			if (m->m_slot_idx == -1)
				m_slot_status			= need_to_attach;
			else if (m->m_slot_idx != s16_max)
			{
				m_slot_status			= attached;
				m_slot_idx				= static_cast<int>(m->m_slot_idx);
				m_slot_pos				= static_cast<int>(m->m_slot_pos);
			}
		}
	}
	
	return								CModule::aboba(type, data, param);
}

void MAddon::startAttaching(CAddonSlot CPC slot)
{
	m_slot_idx							= slot->idx;
	auto act							= get_attach_action();
	act->item_id						= slot->parent_ao->O.ID();
	act->performActionFunctor			();
}

void MAddon::onAttach(CAddonSlot* slot)
{
	m_slot_status						= attached;
	m_slot								= slot;
	m_slot_idx							= slot->idx;
	get_attach_action()->item_id		= u16_max;
}

void MAddon::onDetach(bool transfer)
{
	m_slot_status						= free;
	m_slot								= nullptr;
	m_slot_idx							= no_idx;
	m_slot_pos							= no_idx;

	if (transfer)
	{
		auto root						= O.H_Root();
		O.transfer						((root->scast<CInventoryOwner*>()) ? root->ID() : u16_max);
	}
}

void MAddon::updateHudTransform(Dmatrix CR$ parent_trans)
{
	m_hud_transform.mul					(parent_trans, m_hud_offset);
}

void MAddon::updateHudOffset(Dmatrix CR$ bone_offset, Dmatrix CR$ root_offset)
{
	m_hud_offset.invert					(bone_offset);
	m_hud_offset.mulB_43				(root_offset);
	m_hud_offset.mulB_43				(m_local_transform);
}
