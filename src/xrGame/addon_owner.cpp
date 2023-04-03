#include "stdafx.h"
#include "addon_owner.h"
#include "GameObject.h"
#include "addon.h"
#include "inventory_item.h"
#include "player_hud.h"
#include "HudItem.h"

CAddonOwner::CAddonOwner(CGameObject* obj) : CModule(obj)
{
	m_NextAddonSlot						= u16_max;
	m_Slots.clear						();
	LoadAddonSlots						(pSettings->r_string(O.cNameSect(), "slots"));
}

CAddonOwner::~CAddonOwner()
{
	m_Slots.clear();
}

void CAddonOwner::LoadAddonSlots(LPCSTR section)
{
	shared_str							tmp;
	u16 i								= 0;
	while (pSettings->line_exist(section, tmp.printf("name_%d", i)))
		m_Slots.push_back				(xr_new<SAddonSlot>(section, i++, m_Slots));
}

int CAddonOwner::TransferAddon(CAddon CPC addon, bool attach)
{
	float res							= O.Aboba(eTransferAddon, (void*)addon, (int)attach);
	if (res == flt_max)
	{
		addon->Transfer					((attach) ? O.ID() : ((O.H_Parent()) ? O.H_Parent()->ID() : u16_max));
		return							2;
	}
	return								(int)res;
}

float CAddonOwner::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eOnChild:
		{
			CAddon* addon				= cast<CAddon*>((CObject*)data);
			if (!addon)
				break;

			u16* idx					= NULL;
			if (param)
			{
				idx						= &addon->m_ItemCurrPlace.value;
				if (m_NextAddonSlot != u16_max)
				{
					*idx				= m_NextAddonSlot;
					m_NextAddonSlot		= u16_max;
				}
			}
			else
			{
				for (auto s : m_Slots)
				{
					if (s->addon == addon)
					{
						idx				= &s->idx;
						break;
					}
				}
			}

			if (idx)
			{
				if (param)
					m_Slots[*idx]->addon = addon;
				O.Aboba					(eOnAddon, (void*)m_Slots[*idx], param);
				if (!param)
					m_Slots[*idx]->addon = NULL;
			}
			break;
		}
		case eWeight:
		case eVolume:
		case eCost:
		{
			float res					= 0.f;
			for (auto slot : m_Slots)
			{
				if (slot->addon)
					res					+= slot->addon->Aboba(type);
			}
			return						res;
		}
	}

	return								CModule::aboba(type, data, param);
}

void CAddonOwner::ModifyControlInertionFactor C$(float& cif)
{
	for (auto& slot : m_Slots)
	{
		if (slot->addon)
			cif							*= slot->addon->GetControlInertionFactor();
	}
}

int CAddonOwner::AttachAddon(CAddon CPC addon, u16 slot_idx)
{
	if (!addon)
		return							0;

	if (slot_idx == u16_max)
	{
		for (auto slot : m_Slots)
		{
			if (slot->CanTake(addon))
			{
				slot_idx				= slot->idx;
				break;
			}
		}
	}

	if (slot_idx != u16_max)
	{
		m_NextAddonSlot					= slot_idx;
		return							TransferAddon(addon, true);
	}

	return								0;
}

int CAddonOwner::DetachAddon(CAddon CPC addon)
{
	return TransferAddon				(addon, false);
}

void CAddonOwner::UpdateSlotsTransform()
{
	attachable_hud_item* hi				= smart_cast<CHudItem*>(&O)->HudItemData();
	for (auto slot : m_Slots)
		slot->UpdateRenderPos			(hi->m_model->dcast_RenderVisual(), hi->m_item_transform);
}

void CAddonOwner::render_hud_mode()
{
	for (auto& slot : m_Slots)
		slot->RenderHud					();
}

void CAddonOwner::renderable_Render()
{
	for (auto slot : m_Slots)
		slot->RenderWorld				(O.Visual(), O.XFORM());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SAddonSlot::SAddonSlot(LPCSTR section, u16 _idx, VSlots CR$ slots) : parent_slots(slots)
{
	idx									= _idx;
	shared_str							tmp;

	tmp.printf							("name_%d", idx);
	name								= pSettings->r_string(section, *tmp);

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("attach_bone_%d", idx);
	bone_name							= READ_IF_EXISTS(pSettings, r_string, section, *tmp, "wpn_body");

	tmp.printf							("model_offset_pos_%d", idx);
	model_offset[0]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);

	tmp.printf							("bone_offset_%d", idx);
	if (pSettings->line_exist(section, *tmp))
		model_offset[0].sub				(pSettings->r_fvector3(section, *tmp));

	tmp.printf							("model_offset_rot_%d", idx);
	model_offset[1]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);

	tmp.printf							("icon_offset_pos_%d", idx);
	icon_offset							= READ_IF_EXISTS(pSettings, r_fvector2, section, *tmp, vZero2);

	tmp.printf							("magazine_%d", idx);
	magazine							= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("lower_ironsights_%d", idx);
	lower_iron_sights					= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("primary_scope_%d", idx);
	primary_scope						= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("overlaping_slot_%d", idx);
	overlaping_slot						= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, u16_max);

	addon = loading_addon				= NULL;
	render_pos.identity					();
}

void SAddonSlot::UpdateRenderPos(IRenderVisual* model, Fmatrix parent)
{
	u16 bone_id							= model->dcast_PKinematics()->LL_BoneID(bone_name);
	Fmatrix bone_trans					= model->dcast_PKinematics()->LL_GetTransform(bone_id);

	render_pos.identity					();
	render_pos.mul						(parent, bone_trans);

	Fmatrix								hud_rotation;
	hud_rotation.identity				();
	hud_rotation.rotateX				(model_offset[1].x);

	Fmatrix								hud_rotation_y;
	hud_rotation_y.identity				();
	hud_rotation_y.rotateY				(model_offset[1].y);
	hud_rotation.mulA_43				(hud_rotation_y);

	hud_rotation_y.identity				();
	hud_rotation_y.rotateZ				(model_offset[1].z);
	hud_rotation.mulA_43				(hud_rotation_y);

	hud_rotation.translate_over			(model_offset[0]);
	render_pos.mulB_43					(hud_rotation);
}

void SAddonSlot::RenderHud()
{
	if (addon)
		addon->Render					(&render_pos);

	if (loading_addon)
		loading_addon->Render			(&render_pos);
}

void SAddonSlot::RenderWorld(IRenderVisual* model, Fmatrix parent)
{
	if (addon)
	{
		Fmatrix backup					= render_pos;
		UpdateRenderPos					(model, parent);
		addon->Render					(&render_pos);
		render_pos						= backup;
	}
}

bool SAddonSlot::Compatible(CAddon CPC _addon) const
{
	if (_addon->SlotType() != type)
		return							false;

	if (overlaping_slot != u16_max && parent_slots[overlaping_slot]->addon)
		return							false;

	return								true;
}

bool SAddonSlot::CanTake(CAddon CPC _addon) const
{
	return								Compatible(_addon) && !addon;
}
