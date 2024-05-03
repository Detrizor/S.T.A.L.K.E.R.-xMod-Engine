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
		slots.push_back					(xr_new<CAddonSlot>(section, i++, parent_ao));
}

void CAddonOwner::calculateSlotsBoneOffset()
{
	auto hi								= O.Cast<CHudItem*>()->HudItemData();
	for (auto s : m_Slots)
		s->calculateBoneOffset			(hi->m_model);
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
				s->RenderWorld			(O.XFORM());
			break;
		case eOnChild:
			if (auto addon = cast<CAddon*>((CObject*)data))
			{
				auto slot				= (addon->getSlotIdx() != u16_max) ? m_Slots[addon->getSlotIdx()] : nullptr;
				if (!slot)
				{
					slot				= find_available_slot(addon);
					R_ASSERT			(slot);
					addon->setSlotIdx	(slot->idx);
				}
				if (param)
					slot->attachAddon	(addon);
				RegisterAddon			(addon, param);
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
					res					+= addon->O.Aboba(type);
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

CAddonSlot* CAddonOwner::find_available_slot(CAddon* addon) const
{
	for (auto s : m_Slots)
	{
		if (s->CanTake(addon))
			return						s;
	}
	return								NULL;
}

int CAddonOwner::AttachAddon(CAddon* addon, CAddonSlot* slot)
{
	if (!slot)
		slot							= find_available_slot(addon);

	if (slot)
	{
		addon->setSlotIdx				(slot->idx);
		return							TransferAddon(addon, true);
	}

	return								0;
}

int CAddonOwner::DetachAddon(CAddon* addon)
{
	return TransferAddon				(addon, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAddonSlot::CAddonSlot(LPCSTR section, u16 _idx, CAddonOwner PC$ parent):
	m_parent_addon(parent->cast<CAddon*>()),
	parent_ao(parent),
	idx(_idx)
{
	shared_str							tmp;

	tmp.printf							("name_%d", idx);
	name								= CStringTable().translate(pSettings->r_string(section, *tmp));

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("length_%d", idx);
	float length						= READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.f);

	tmp.printf							("step_%d", idx);
	if (pSettings->line_exist(section, tmp))
	{
		m_step							= pSettings->r_float(section, *tmp);
		steps							= (int)roundf(length / m_step);
	}
	else
	{
		tmp.printf						("steps_%d", idx);
		steps							= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 1);
		m_step							= length / (float(steps) + .5f);
	}

	tmp.printf							("model_offset_rot_%d", idx);
	model_offset.setHPBv				(READ_IF_EXISTS(pSettings, r_fvector3d2r, section, *tmp, vZero));
	tmp.printf							("model_offset_pos_%d", idx);
	model_offset.translate_over			(READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero));

	tmp.printf							("icon_offset_pos_%d", idx);
	icon_offset							= READ_IF_EXISTS(pSettings, r_fvector2, section, *tmp, vZero2);
	
	tmp.printf							("icon_length_%d", idx);
	float icon_length					= READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.f);
	icon_step							= icon_length / (float(steps) + .5f);

	tmp.printf							("blocking_ironsights_%d", idx);
	blocking_iron_sights				= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 0);

	tmp.printf							("overlapping_slot_%d", idx);
	m_overlaping_slot					= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, u16_max);
	
	tmp.printf							("muzzle_%d", idx);
	muzzle								= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("has_loading_anim_%d", idx);
	m_has_loading_anim					= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);

	tmp.printf							("slide_attach_%d", idx);
	m_slide_attach						= READ_IF_EXISTS(pSettings, r_bool, section, *tmp, false);
}

void CAddonSlot::append_bone_trans(Fmatrix& trans, IKinematics* model) const
{
	if (m_bone_id != u16_max)
	{
		trans.mulB_43					(model->LL_GetTransform(m_bone_id));
		trans.mulB_43					(m_bone_offset);
	}
}

int CAddonSlot::get_spacing(CAddon CPC left, CAddon CPC right) const
{
	if (m_step == 0.f)
		return							0;
	int mount_edge_bonus				= (m_parent_addon) ? -1 : 0;
	if (!left)
		return							mount_edge_bonus;
	if (!right)
		return							left->getLength(m_step) + mount_edge_bonus;
	if (left->isLowProfile() || right->isLowProfile())
		return							left->getLength(m_step);
	return								left->getLength(m_step, CAddon::ProfileFwd) + right->getLength(m_step, CAddon::ProfileBwd);
}

CAddon* CAddonSlot::get_next_addon(xr_list<CAddon*>::iterator& I) const
{
	return								(++I == addons.end()) ? NULL : *I;
}

CAddon* CAddonSlot::get_prev_addon(xr_list<CAddon*>::iterator& I) const
{
	return								(I == addons.begin()) ? NULL : *--I;
}

void CAddonSlot::attachAddon(CAddon* addon)
{
	addon->setSlot						(this);

	if (addon->getSlotPos() == s16_max)
	{
		if (addon->isFrontPositioning())
		{
			auto I						= addons.end();
			int pos						= steps - get_spacing(addon, NULL);
			while (auto prev = get_prev_addon(I))
			{
				if (pos >= prev->getSlotPos() + get_spacing(prev, addon))
				{
					I++;
					break;
				}
				pos						= prev->getSlotPos() - get_spacing(addon, prev);
			}
			addons.insert				(I, addon);
			addon->setSlotPos			(pos);
		}
		else
		{
			auto I						= --addons.begin();
			int pos						= get_spacing(NULL, addon);
			while (auto next = get_next_addon(I))
			{
				if (pos + get_spacing(addon, next) <= next->getSlotPos())
					break;
				pos						= next->getSlotPos() + get_spacing(next, addon);
			}
			addons.insert				(I, addon);
			addon->setSlotPos			(pos);
		}
	}
	else
	{
		auto I							= addons.begin();
		while (auto next = get_next_addon(I))
			if (addon->getSlotPos() < next->getSlotPos())
				break;
		addons.insert					(I, addon);
	}

	updateAddonLocalTransform			(addon);
}

void CAddonSlot::detachAddon(CAddon* addon)
{
	addon->setSlot						(NULL);
	addon->setSlotIdx					(u16_max);
	addon->setSlotPos					(s16_max);
	addons.erase						(::std::find(addons.begin(), addons.end(), addon));
}

void CAddonSlot::shiftAddon(CAddon* addon, int shift)
{
	auto A								= ::std::find(addons.begin(), addons.end(), addon);
	auto I								= A;
	int pos								= addon->getSlotPos();
	int prev_pos						= pos;
	if (shift > 0)
	{
		auto next						= get_next_addon(I);
		while (shift--)
		{
			int npos					= pos + 1;
			int lim_pos					= (next) ? next->getSlotPos() : steps;
			if (npos + get_spacing(addon, next) <= lim_pos)
				pos						= npos;
			else while (next)
			{
				npos					= next->getSlotPos() + get_spacing(next, addon);
				next					= get_next_addon(I);
				lim_pos					= (next) ? next->getSlotPos() : steps;
				if (npos + get_spacing(addon, next) <= lim_pos)
				{
					pos					= npos;
					addons.erase		(A);
					A					= addons.insert(I, addon);
					break;
				}
			}
			if (prev_pos == pos)
				break;
			else
				prev_pos				= pos;
		}
	}
	else
	{
		auto prev						= get_prev_addon(I);
		while (shift++)
		{
			int npos					= pos - 1;
			int lim_pos					= (prev) ? prev->getSlotPos(): 0;
			if (npos >= lim_pos + get_spacing(prev, addon))
				pos						= npos;
			else while (prev)
			{
				npos					= prev->getSlotPos() - get_spacing(addon, prev);
				prev					= get_prev_addon(I);
				lim_pos					= (prev) ? prev->getSlotPos() : 0;
				if (npos >= lim_pos + get_spacing(prev, addon))
				{
					pos					= npos;
					addons.erase		(A);
					A					= addons.insert((prev) ? ++I : I, addon);
					I					= A;
					if (prev)
						I--;
					break;
				}
			}
			if (prev_pos == pos)
				break;
			else
				prev_pos				= pos;
		}
	}

	if (pos != addon->getSlotPos())
	{
		addon->setSlotPos				(pos);
		updateAddonLocalTransform		(addon);
	}
}

void CAddonSlot::updateAddonsHudTransform(IKinematics* model, Fmatrix CR$ parent_trans)
{
	for (auto addon : addons)
	{
		Fmatrix trans					= parent_trans;
		append_bone_trans				(trans, model);
		addon->updateHudTransform		(trans);
		if (auto ao = addon->cast<CAddonOwner*>())
			for (auto s : ao->AddonSlots())
				s->updateAddonsHudTransform(parent_trans);
	}
}

void CAddonSlot::updateAddonsHudTransform(Fmatrix CR$ parent_trans)
{
	for (auto addon : addons)
	{
		addon->updateHudTransform		(parent_trans);
		if (auto ao = addon->cast<CAddonOwner*>())
			for (auto s : ao->AddonSlots())
				s->updateAddonsHudTransform(parent_trans);
	}
}

void CAddonSlot::startReloading(CAddon* loading_addon)
{
	m_loading_addon						= loading_addon;
}

void CAddonSlot::loadingDetach()
{
	for (auto a : addons)
		a->Transfer						(parent_ao->O.H_Parent()->ID());
}

void CAddonSlot::loadingAttach()
{
	if (m_loading_addon)
		m_loading_addon->Transfer		(parent_ao->O.ID());
}

void CAddonSlot::finishLoading(bool interrupted)
{
	if (interrupted)
		for (auto a : addons)
			a->Transfer					(u16_max);
	if (m_loading_addon)
		m_loading_addon					= nullptr;
}

void CAddonSlot::RenderHud() const
{
	for (auto addon : addons)
	{
		addon->RenderHud				();
		if (auto ao = addon->cast<CAddonOwner CP$>())
			for (auto s : ao->AddonSlots())
				s->RenderHud			();
	}
}

void CAddonSlot::calculateBoneOffset(IKinematics* model)
{
	if (m_has_loading_anim)
		m_bone_id						= model->LL_BoneID(pSettings->r_string(parent_ao->cast<CHudItem*>()->HudSection(), "loading_bone_name"));
	else if (m_slide_attach)
		m_bone_id						= model->LL_BoneID(pSettings->r_string(parent_ao->cast<CHudItem*>()->HudSection(), "slide_bone_name"));
	else
		return;

	m_bone_offset						= model->LL_GetTransform(m_bone_id);
	m_bone_offset.invert				();
}

void CAddonSlot::RenderWorld(Fmatrix CR$ parent_trans) const
{
	for (auto addon : addons)
	{
		addon->RenderWorld				(parent_trans);
		if (auto ao = addon->cast<CAddonOwner CP$>())
			for (auto s : ao->AddonSlots())
				s->RenderWorld			(parent_trans);
	}
}

bool CAddonSlot::Compatible(CAddon CPC addon) const
{
	if (addon->SlotType() != type)
	{
		if (!pSettings->line_exist("slot_exceptions", addon->SlotType()))
			return						false;
		
		if (strcmp(pSettings->r_string("slot_exceptions", *addon->SlotType()), *type))
			return						false;
	}

	if (m_overlaping_slot != u16_max && parent_ao->AddonSlots()[m_overlaping_slot]->addons.size())
		return							false;

	return								true;
}

bool CAddonSlot::CanTake(CAddon CPC addon) const
{
	if (!Compatible(addon))
		return							false;
	if (m_has_loading_anim)
		return							!isLoading();

	int pos								= get_spacing(NULL, addon);
	for (auto next : addons)
	{
		if (pos + get_spacing(addon, next) <= next->getSlotPos())
			return						true;
		pos								= next->getSlotPos() + get_spacing(next, addon);
	}
	
	return								(pos + get_spacing(addon, NULL) <= steps);
}

void CAddonSlot::updateAddonLocalTransform(CAddon* addon) const
{
	Fmatrix transform					= model_offset;
	transform.c.z						+= float(addon->getSlotPos()) * m_step;
	addon->updateLocalTransform			((m_parent_addon) ? transform.mulA_43(m_parent_addon->getLocalTransform()) : transform);
	if (auto ao = addon->cast<CAddonOwner*>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				s->updateAddonLocalTransform(a);
}
