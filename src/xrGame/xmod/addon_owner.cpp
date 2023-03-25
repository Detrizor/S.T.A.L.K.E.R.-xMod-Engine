#include "stdafx.h"
#include "../player_hud.h"

#include "addon_owner.h"
#include "addon.h"

CAddonOwner::CAddonOwner()
{
	m_Slots.clear							();
	m_NextAddonSlot							= NO_ID;
}

DLL_Pure* CAddonOwner::_construct()
{
	inherited::_construct					();
	m_object								= smart_cast<CGameObject*>(this);
	return									m_object;
}

void CAddonOwner::Load(LPCSTR section)
{
	if (pSettings->line_exist				(section, "slots"))
		LoadAddonSlots						(pSettings->r_string(section, "slots"));
}

void CAddonOwner::LoadAddonSlots(LPCSTR section)
{
	shared_str								tmp;
	u16 i									= 0;
	while (pSettings->line_exist(section, tmp.printf("name_%d", i++)))
	{
		SAddonSlot addon_slot				= SAddonSlot(pSettings->r_string(section, *tmp), m_Slots);
		addon_slot.Load						(section, i);
		m_Slots.push_back					(addon_slot);
	}
}

void CAddonOwner::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	inherited::OnEventImpl						(type, id, itm, dont_create_shell);
	CAddonObject* addon							= smart_cast<CAddonObject*>(itm);
	if (!addon)
		return;

	u16& idx									= addon->m_ItemCurrPlace.value;
	if (m_NextAddonSlot != NO_ID)
	{
		idx										= m_NextAddonSlot;
		m_NextAddonSlot							= NO_ID;
	}
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		m_Slots[idx].addon						= addon;
		AddonAttach								(addon);
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		m_Slots[idx].addon						= NULL;
		AddonDetach								(addon);
		break;
	}
}

void CAddonOwner::AttachAddon(CAddonObject* addon, u16 slot_idx)
{
	if (!addon)
		return;

	if (slot_idx == NO_ID)
	{
		for (auto& slot : m_Slots)
		if (slot.CanTake(addon))
		{
			slot_idx							= slot.idx;
			break;
		}
	}

	if (slot_idx != NO_ID)
	{
		m_NextAddonSlot							= slot_idx;
		TransferAnimation						(addon, true);
	}
}

void CAddonOwner::DetachAddon(CAddonObject* addon)
{
	TransferAnimation							(addon, false);
}

void CAddonOwner::TransferAnimation(CAddonObject* addon, bool attach)
{
	addon->Transfer								((attach) ? m_object->ID() : m_object->H_Parent()->ID());
}

void CAddonOwner::renderable_Render()
{
	for (auto& slot : m_Slots)
		if (slot.addon)
			slot.addon->Update_And_Render_World	(m_object->Visual(), m_object->XFORM());
}

void CAddonOwner::UpdateAddonsTransform()
{
	attachable_hud_item* hi						= smart_cast<CHudItem*>(m_object)->HudItemData();
	if (!hi)
		return;

	for (auto& slot : m_Slots)
		if (slot.addon)
			slot.addon->UpdateRenderPos			(hi->m_model->dcast_RenderVisual(), hi->m_item_transform);
}

void CAddonOwner::render_hud_mode()
{
	for (auto& slot : m_Slots)
		if (slot.addon)
			slot.addon->Render					();
}

float CAddonOwner::GetControlInertionFactor() const
{
	float res									= 1.f;
	for (auto& slot : m_Slots)
		if (slot.addon)
			res									*= slot.addon->GetControlInertionFactor();
	return										res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SAddonSlot::SAddonSlot(LPCSTR _name, const xr_vector<SAddonSlot>& slots) : parent_slots(slots)
{
	name = _name;
	model_offset[0] = model_offset[1] = { 0.f, 0.f, 0.f };
	icon_offset = { 0.f, 0.f };
	icon_on_background = false;
	lower_iron_sights = false;
	primary_scope = false;
	overlaping_slot = NO_ID;
	addon = NULL;
}

void SAddonSlot::Load(LPCSTR section, u16 _idx)
{
	idx							= _idx;
	shared_str					tmp;
	tmp.printf					("type_%d", idx);
	type						= pSettings->r_string(section, *tmp);

	tmp.printf("model_offset_pos_%d", idx);
	if (pSettings->line_exist(section, tmp))
		model_offset[0]			= pSettings->r_fvector3(section, *tmp);

	tmp.printf("model_offset_rot_%d", idx);
	if (pSettings->line_exist(section, tmp))
		model_offset[1]			= pSettings->r_fvector3(section, *tmp);

	tmp.printf("icon_offset_pos_%d", idx);
	if (pSettings->line_exist(section, tmp))
		icon_offset				= pSettings->r_fvector2(section, *tmp);

	tmp.printf("icon_on_background_%d", idx);
	if (pSettings->line_exist(section, tmp))
		icon_on_background		= !!pSettings->r_bool(section, *tmp);

	tmp.printf("lower_ironsights_%d", idx);
	if (pSettings->line_exist(section, tmp))
		lower_iron_sights		= !!pSettings->r_bool(section, *tmp);

	tmp.printf("primary_scope_%d", idx);
	if (pSettings->line_exist(section, tmp))
		primary_scope			= !!pSettings->r_bool(section, *tmp);

	tmp.printf("overlaping_slot_%d", idx);
	if (pSettings->line_exist(section, tmp))
		overlaping_slot			= pSettings->r_u16(section, *tmp);
}

bool SAddonSlot::CanTake(CAddonObject CPC _addon) const
{
	if (addon)
		return false;

	if (_addon->SlotType() != type)
		return false;

	if (overlaping_slot != NO_ID && parent_slots[overlaping_slot].addon)
		return false;

	return true;
}
