#include "stdafx.h"
#include "addon_owner.h"
#include "GameObject.h"
#include "addon.h"
#include "inventory_item.h"
#include "player_hud.h"
#include "HudItem.h"
#include "string_table.h"
#include "../xrEngine/CameraManager.h"
#include "WeaponMagazined.h"

CAddonOwner::CAddonOwner(CGameObject* obj) : CModule(obj)
{
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
		if (attach)
			addon->Transfer				(O.ID());
		else
		{
			auto parent					= O.H_Parent();
			if (parent)
				while (parent->H_Parent())
					parent				= parent->H_Parent();
			addon->Transfer				((parent && parent->Cast<CInventoryOwner*>()) ? parent->ID() : u16_max);
		}
		return							2;
	}
	return								(int)res;
}

float CAddonOwner::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eRenderableRender:
			for (auto s : m_Slots)
				s->RenderWorld			(O.Visual(), O.XFORM());
			break;
		case eOnChild:
			if (auto addon = cast<CAddon*>((CObject*)data))
			{
				SAddonSlot* slot		= m_Slots[(int)addon->getSlot()];
				if (param)
					slot->attachAddon	(addon);
				RegisterAddon			(addon, !!param);
				if (!param)
					slot->detachAddon	(addon);
			}
			break;
		case eWeight:
		case eVolume:
		case eCost:
		{
			float res					= 0.f;
			for (auto slot : m_Slots)
				for (auto addon : slot->addons)
					res					+= addon->Aboba(type);
			return						res;
		}
		case eRenderHudMode:
			for (auto s : m_Slots)
				s->RenderHud			();
			break;
		case eUpdateSlotsTransform:
		{
			attachable_hud_item* hi		= O.Cast<CHudItem*>()->HudItemData();
			for (auto s : m_Slots)
				s->updateAddonsHudTransform(hi->m_model, hi->m_transform);
			break;
		}
	}

	return								CModule::aboba(type, data, param);
}

CAddonOwner* CAddonOwner::getParentAO() const
{
	return								(O.H_Parent()) ? O.H_Parent()->Cast<CAddonOwner*>() : NULL;
}

void CAddonOwner::RegisterAddon(CAddon PC$ addon, bool attach)
{
	if (auto parent_ao = getParentAO())
		parent_ao->RegisterAddon		(addon, attach);
	else
		O.Aboba							(eOnAddon, (void*)addon, attach);
}

int CAddonOwner::AttachAddon(CAddon* addon, SAddonSlot* slot)
{
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
		addon->setSlot					((float)slot->idx);
		return							TransferAddon(addon, true);
	}

	return								0;
}

int CAddonOwner::DetachAddon(CAddon* addon)
{
	return TransferAddon				(addon, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SAddonSlot::SAddonSlot(LPCSTR section, u16 _idx, CAddonOwner PC$ parent):
	parent_ao(parent),
	parent_addon(parent->cast<CAddon*>())
{
	idx									= _idx;
	shared_str							tmp;

	tmp.printf							("name_%d", idx);
	name								= CStringTable().translate(pSettings->r_string(section, *tmp));

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("length_%d", idx);
	length								= READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.f);

	tmp.printf							("steps_%d", idx);
	steps								= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 0);

	tmp.printf							("attach_bone_%d", idx);
	auto obj							= parent->cast<CObject*>();
	shared_str bone_name				= READ_IF_EXISTS(pSettings, r_string, section, *tmp, "wpn_body");
	bone_id								= obj->Visual()->dcast_PKinematics()->LL_BoneID(bone_name);

	tmp.printf							("model_offset_rot_%d", idx);
	model_offset.setHPBv				(READ_IF_EXISTS(pSettings, r_fvector3d2r, section, *tmp, vZero));
	tmp.printf							("model_offset_pos_%d", idx);
	model_offset.translate_over			(READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero));
	
	tmp.printf							("bone_offset_rot_%d", idx);
	bone_offset.setXYZi					(READ_IF_EXISTS(pSettings, r_fvector3d2r, section, *tmp, vZero));
	tmp.printf							("bone_offset_pos_%d", idx);
	bone_offset.translate_over			(READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero));

	tmp.printf							("icon_offset_pos_%d", idx);
	icon_offset							= READ_IF_EXISTS(pSettings, r_fvector2, section, *tmp, vZero2);

	tmp.printf							("blocking_ironsights_%d", idx);
	blocking_iron_sights				= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("overlapping_slot_%d", idx);
	overlaping_slot						= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, u16_max);
	
	tmp.printf							("muzzle_%d", idx);
	muzzle								= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("magazine_%d", idx);
	magazine							= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);
	if (magazine)
	{
		tmp.printf						("loading_attach_bone_%d", idx);
		if (pSettings->line_exist(section, *tmp))
		{
			bone_name					= pSettings->r_string(section, *tmp);
			loading_bone_id				= obj->Visual()->dcast_PKinematics()->LL_BoneID(bone_name);
		}
		else
			loading_bone_id				= bone_id;
		
		loading_model_offset			= model_offset;
		tmp.printf						("loading_model_offset_rot_%d", idx);
		if (pSettings->line_exist(section, *tmp))
			loading_model_offset.setHPBv(pSettings->r_fvector3d2r(section, *tmp));
		tmp.printf						("loading_model_offset_pos_%d", idx);
		if (pSettings->line_exist(section, *tmp))
			loading_model_offset.translate_over(pSettings->r_fvector3(section, *tmp));

		loading_bone_offset				= bone_offset;
		tmp.printf						("loading_bone_offset_rot_%d", idx);
		if (pSettings->line_exist(section, *tmp))
			loading_bone_offset.setXYZi	(pSettings->r_fvector3d2r(section, *tmp));
		tmp.printf						("loading_bone_offset_pos_%d", idx);
		if (pSettings->line_exist(section, *tmp))
			loading_bone_offset.translate_over(pSettings->r_fvector3(section, *tmp));
		loading_bone_offset.invert		();
	}
	bone_offset.invert					();
}

void SAddonSlot::append_bone_trans(Fmatrix& trans, IKinematics* model, Fmatrix CPC parent_trans, u16 bone) const
{
	trans								= model->LL_GetTransform(bone);
	if (parent_trans)
		trans.mulA_43					(*parent_trans);
}

void SAddonSlot::updateAddonsHudTransform(IKinematics* model, Fmatrix CR$ parent_trans)
{
	auto update_impl = [this, model, parent_trans](CAddon* addon, u16 bone_id_)
	{
		Fmatrix							trans;
		append_bone_trans				(trans, model, &parent_trans, bone_id_);
		addon->updateHudTransform		(trans);
	};

	for (auto addon : addons)
		update_impl						(addon, bone_id);

	if (loading_addon)
		update_impl						(loading_addon, loading_bone_id);
}

void SAddonSlot::RenderHud() const
{
	for (auto addon : addons)
		addon->RenderHud				();
	
	if (loading_addon)
		loading_addon->RenderHud		();
}

void SAddonSlot::RenderWorld(IRenderVisual* model, Fmatrix CR$ parent_trans) const
{
	auto render_impl = [this, model, parent_trans](CAddon* addon, u16 bone_id_)
	{
		Fmatrix							trans;
		append_bone_trans				(trans, model->dcast_PKinematics(), &parent_trans, bone_id_);
		addon->RenderWorld				(trans);
	};

	for (auto addon : addons)
		render_impl						(addon, bone_id);
	if (loading_addon)
		render_impl						(loading_addon, loading_bone_id);
}

void SAddonSlot::attachAddon(CAddon* addon)
{
	addons.push_back					(addon);
	addon->setOwner						(parent_ao);
	updateAddonLocalTransform			(addon);
}

void SAddonSlot::attachLoadingAddon(CAddon* addon)
{
	loading_addon						= addon;
	loading_transform.identity			();
	loading_transform.mulB_43			(loading_bone_offset);
	loading_transform.mulB_43			(loading_model_offset);

	loading_addon->updateLocalTransform	(&loading_transform);
	loading_addon->setRootBoneID		(loading_bone_id);
}

void SAddonSlot::updateAddonLocalTransform(CAddon* addon)
{
	if (parent_addon)
		append_bone_trans				(transform, parent_addon->Visual()->dcast_PKinematics(), NULL, bone_id);
	else
		transform.identity				();
	transform.mulB_43					(bone_offset);
	transform.mulB_43					(model_offset);

	if (parent_addon)
	{
		Fmatrix							trans;
		trans.mul						(parent_addon->getLocalTransform(), transform);
		addon->updateLocalTransform		(&trans);
		addon->setRootBoneID			(parent_addon->getRootBoneID());
	}
	else
	{
		addon->updateLocalTransform		(&transform);
		addon->setRootBoneID			(bone_id);
	}
}

void SAddonSlot::detachAddon(CAddon* addon)
{
	addon->updateLocalTransform			(NULL);
	addon->setOwner						(NULL);
	addon->setSlot						(-1.f);
	auto I								= ::std::find(addons.begin(), addons.end(), addon);
	if (I != addons.end())
		addons.erase					(I);
}

void SAddonSlot::detachLoadingAddon()
{
	loading_addon						= NULL;
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

	if (overlaping_slot != u16_max && parent_ao->AddonSlots()[overlaping_slot]->addons.size())
		return							false;

	return								true;
}

bool SAddonSlot::CanTake(CAddon CPC _addon) const
{
	return								Compatible(_addon) && (addons.empty() || magazine);
}
