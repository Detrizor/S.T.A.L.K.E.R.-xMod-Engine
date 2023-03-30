#include "stdafx.h"
#include "addon_owner.h"
#include "inventory_space.h"
#include "GameObject.h"
#include "addon.h"
#include "inventory_item.h"
#include "player_hud.h"
#include "HudItem.h"

CAddonOwner::CAddonOwner(CGameObject* obj) : CModule(obj)
{
	m_NextAddonSlot						= NO_ID;
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

void CAddonOwner::OnChild(CObject* obj, bool take)
{
	CAddon* addon						= obj->cast<CAddon*>();
	if (!addon)
		return;

	u16* idx							= NULL;
	if (take)
	{
		PIItem item						= smart_cast<PIItem>(obj);
		R_ASSERT						(item);
		idx								= &item->m_ItemCurrPlace.value;
		if (m_NextAddonSlot != NO_ID)
		{
			*idx						= m_NextAddonSlot;
			m_NextAddonSlot				= NO_ID;
		}
	}
	else
	{
		for (const auto s : m_Slots)
		{
			if (s->addon == addon)
			{
				idx						= &s->idx;
				break;
			}
		}
	}

	if (idx)
	{
		m_Slots[*idx]->addon			= (take) ? addon : NULL;
		O.ProcessAddon					(addon, take, m_Slots[*idx]);
	}
}

bool CAddonOwner::AttachAddon(CAddon CPC addon, u16 slot_idx)
{
	if (!addon)
		return							false;

	if (slot_idx == NO_ID)
	{
		for (const auto slot : m_Slots)
		{
			if (slot->CanTake(addon))
			{
				slot_idx				= slot->idx;
				break;
			}
		}
	}

	if (slot_idx != NO_ID)
	{
		m_NextAddonSlot					= slot_idx;
		O.TransferAddon					(addon, true);
		return							true;
	}

	return								false;
}

void CAddonOwner::DetachAddon(CAddon CPC addon)
{
	O.TransferAddon						(addon, false);
}

void CAddonOwner::UpdateSlotsTransform()
{
	attachable_hud_item* hi				= smart_cast<CHudItem*>(pO)->HudItemData();
	if (!hi)
		return;

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

void CAddonOwner::ModifyControlInertionFactor C$(float& cif)
{
	for (auto& slot : m_Slots)
	{
		if (slot->addon)
		{
			CInventoryItem CPC item		= smart_cast<CInventoryItem CP$>(&slot->addon->Object());
			if (item)
				cif						*= item->GetControlInertionFactor();
		}
	}
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

	tmp.printf							("icon_on_background_%d", idx);
	icon_on_background					= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("lower_ironsights_%d", idx);
	lower_iron_sights					= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("primary_scope_%d", idx);
	primary_scope						= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("overlaping_slot_%d", idx);
	overlaping_slot						= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, NO_ID);

	addon								= NULL;
	render_pos.identity					();
}

bool SAddonSlot::CanTake(CAddon CPC _addon) const
{
	if (addon)
		return							false;

	if (_addon->SlotType() != type)
		return							false;

	if (overlaping_slot != NO_ID && parent_slots[overlaping_slot]->addon)
		return							false;

	return								true;
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
