#include "stdafx.h"
#include "addon_owner.h"
#include "GameObject.h"
#include "addon.h"
#include "inventory_item.h"
#include "player_hud.h"
#include "HudItem.h"
#include "string_table.h"

CAddonOwner::CAddonOwner(CGameObject* obj) : CModule(obj)
{
	m_NextAddonSlot						= NULL;
	m_Slots.clear						();
	LoadAddonSlots						(pSettings->r_string(O.cNameSect(), "slots"));
}

CAddonOwner::~CAddonOwner()
{
	m_Slots.clear						();
}

void CAddonOwner::LoadAddonSlots(LPCSTR section)
{
	LoadAddonSlots						(section, m_Slots, this);
}

void CAddonOwner::LoadAddonSlots(LPCSTR section, VSlots& slots, CAddonOwner PC$ parent_ao)
{
	shared_str							tmp;
	u16 i								= 0;
	while (pSettings->line_exist(section, tmp.printf("name_%d", i)))
		slots.push_back					(xr_new<SAddonSlot>(section, i++, parent_ao));
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
		case eRenderableRender:
			for (auto s : m_Slots)
				s->RenderWorld			(O.Visual(), O.XFORM());
			break;
		case eOnChild:
		{
			CAddon* addon				= cast<CAddon*>((CObject*)data);
			if (!addon)
				break;

			SAddonSlot* slot			= NULL;
			if (param)
			{
				if (m_NextAddonSlot)
				{
					addon->m_ItemCurrPlace.value = m_NextAddonSlot->idx;
					slot				= m_NextAddonSlot;
					m_NextAddonSlot		= NULL;
				}
				else
					slot				= m_Slots[addon->m_ItemCurrPlace.value];
			}
			else
			{
				for (auto s : m_Slots)
				{
					if (s->addon == addon)
					{
						slot			= s;
						break;
					}
				}
			}

			if (slot)
				RegisterAddon			(addon, slot, !!param);

			break;
		}
		case eWeight:
		case eVolume:
		case eCost:
		{
			float res					= 0.f;
			for (auto s : m_Slots)
			{
				if (s->addon)
					res					+= s->addon->Aboba(type);
			}
			return						res;
		}
		case eRenderHudMode:
			for (auto s : m_Slots)
				s->RenderHud			();
			break;
		case eUpdateSlotsTransform:
		{
			attachable_hud_item* hi		= smart_cast<CHudItem*>(&O)->HudItemData();
			for (auto s : m_Slots)
				s->UpdateRenderPos		(hi->m_model->dcast_RenderVisual(), hi->m_item_transform);
			break;
		}
	}

	return								CModule::aboba(type, data, param);
}

CAddonOwner* CAddonOwner::ParentAO() const
{
	return								(O.H_Parent()) ? O.H_Parent()->Cast<CAddonOwner*>() : NULL;
}

void CAddonOwner::RegisterAddon(CAddon PC$ addon, SAddonSlot PC$ slot, bool attach)
{
	if (attach)
		slot->addon						= addon;

	CAddonOwner* parent_ao				= ParentAO();
	CAddonOwner* ao						= addon->cast<CAddonOwner*>();
	if (parent_ao)
	{
		if (!ao)
		{
			for (auto s : parent_ao->AddonSlots())
			{
				if (s->forwarded_slot == slot)
				{
					parent_ao->RegisterAddon(addon, s, attach);
					break;
				}
			}
		}
	}
	else
		O.Aboba							(eOnAddon, (void*)slot, attach);

	if (!attach)
		slot->addon						= NULL;

	if (ao)
	{
		if (attach)
		{
			for (auto s : ao->AddonSlots())
			{
				SAddonSlot* forward_slot = xr_new<SAddonSlot>(s, slot, ao);
				m_Slots.push_back		(forward_slot);
				if (s->addon && !s->addon->cast<CAddonOwner*>())
					RegisterAddon		(s->addon, forward_slot, attach);
			}
		}
		else
		{
			bool						found;
			do
			{
				found					= false;		//--xd not optimal, was hurrying, better redo sometimes
				for (VSlots::iterator I = m_Slots.begin(), E = m_Slots.end(); I != E; I++)
				{
					if ((*I)->parent_ao == ao)
					{
						if ((*I)->addon && !(*I)->addon->cast<CAddonOwner*>())
							RegisterAddon((*I)->addon, *I, attach);
						xr_delete		(*I);
						m_Slots.erase	(I);
						found			= true;
						break;
					}
				}
			}
			while (found);
		}

		if (parent_ao)
		{
			CAddon* self_addon			= cast<CAddon*>();
			for (auto s : parent_ao->AddonSlots())
			{
				if (s->addon == self_addon)
				{
					parent_ao->RegisterAddon(self_addon, s, false);
					parent_ao->RegisterAddon(self_addon, s, true);
					break;
				}
			}
		}
	}
}

int CAddonOwner::AttachAddon(CAddon CPC addon, SAddonSlot* slot)
{
	if (!addon)
		return							0;

	if (!slot)
	{
		for (auto s : m_Slots)
		{
			if (s->CanTake(addon))
			{
				slot					= s;
				break;
			}
		}
	}

	if (slot)
	{
		if (slot->parent_ao != this)
			return						slot->parent_ao->AttachAddon(addon, slot->forwarded_slot);
		m_NextAddonSlot					= slot;
		return							TransferAddon(addon, true);
	}

	return								0;
}

int CAddonOwner::DetachAddon(CAddon CPC addon)
{
	return TransferAddon				(addon, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SAddonSlot::SAddonSlot(LPCSTR section, u16 _idx, CAddonOwner PC$ parent) : parent_ao(parent), forwarded_slot(NULL)
{
	idx									= _idx;
	shared_str							tmp;

	tmp.printf							("name_%d", idx);
	name								= CStringTable().translate(pSettings->r_string(section, *tmp));

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("attach_bone_%d", idx);
	bone_name							= READ_IF_EXISTS(pSettings, r_string, section, *tmp, "wpn_body");

	tmp.printf							("model_offset_pos_%d", idx);
	model_offset[0]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);

	tmp.printf							("model_offset_rot_%d", idx);
	model_offset[1]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);

	tmp.printf							("bone_offset_rot_%d", idx);
	bone_offset[1]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);

	tmp.printf							("bone_offset_pos_%d", idx);
	bone_offset[0]						= READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero);
	bone_offset[0].mul					(-1.f);
	bone_offset[0].rotate				(bone_offset[1]);

	tmp.printf							("icon_offset_pos_%d", idx);
	icon_offset							= READ_IF_EXISTS(pSettings, r_fvector2, section, *tmp, vZero2);

	tmp.printf							("magazine_%d", idx);
	magazine							= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("lower_ironsights_%d", idx);
	lower_iron_sights					= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("alt_scope_%d", idx);
	alt_scope							= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("overlaping_slot_%d", idx);
	overlaping_slot						= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, u16_max);

	addon = loading_addon				= NULL;
	render_pos.identity					();
}

extern void ApplyPivot(Fvector* offset, Fvector CR$ pivot);
extern void ApplyOffset(Fmatrix& trans, Fvector CR$ position, Fvector CR$ rotation);

SAddonSlot::SAddonSlot(SAddonSlot PC$ slot, SAddonSlot CPC root_slot, CAddonOwner PC$ parent) : parent_ao(parent), forwarded_slot(slot)
{
	name.printf							("%s - %s", *CStringTable().translate(root_slot->addon->NameShort()), *CStringTable().translate(slot->name));
	type								= slot->type;
	bone_name							= root_slot->bone_name;
	model_offset[0]						= root_slot->model_offset[0];
	model_offset[1]						= root_slot->model_offset[1];
	ApplyPivot							(model_offset, slot->model_offset[0]);
	model_offset[1].add					(slot->model_offset[1]);
	bone_offset[0]						= root_slot->bone_offset[0];
	bone_offset[1]						= root_slot->bone_offset[1];
	icon_offset							= slot->icon_offset;
	icon_offset.add						(root_slot->icon_offset);
	icon_offset.sub						(root_slot->addon->IconOffset());
	magazine							= false;
	lower_iron_sights					= false;
	alt_scope							= slot->alt_scope;
	overlaping_slot						= slot->overlaping_slot;

	addon								= slot->addon;
	loading_addon						= NULL;
	render_pos.identity					();
}

void SAddonSlot::UpdateRenderPos(IRenderVisual* model, Fmatrix parent)
{
	u16 bone_id							= model->dcast_PKinematics()->LL_BoneID(bone_name);
	Fmatrix bone_trans					= model->dcast_PKinematics()->LL_GetTransform(bone_id);

	render_pos.identity					();
	render_pos.mul						(parent, bone_trans);

	ApplyOffset							(render_pos, bone_offset[0], bone_offset[1]);
	ApplyOffset							(render_pos, model_offset[0], model_offset[1]);
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
	{
		if (!pSettings->line_exist("slot_exceptions", _addon->SlotType()))
			return						false;
		
		if (!strstr(pSettings->r_string("slot_exceptions", *_addon->SlotType()), *type))
			return						false;
	}

	if (overlaping_slot != u16_max && parent_ao->AddonSlots()[overlaping_slot]->addon)
		return							false;

	return								true;
}

bool SAddonSlot::CanTake(CAddon CPC _addon) const
{
	return								Compatible(_addon) && (!addon || magazine);
}
