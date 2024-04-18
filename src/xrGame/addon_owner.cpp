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
				auto slot				= (addon->getSlotIdx() != -1) ? m_Slots[addon->getSlotIdx()] : NULL;
				if (!slot && param)
				{
					slot				= find_available_slot(addon);
					if (slot)
						addon->setSlotIdx(slot->idx);
				}
				if (slot)
				{
					if (param)
						slot->attachAddon(addon);
					RegisterAddon		(addon, !!param);
					if (!param)
						slot->detachAddon(addon);
				}
				else if (param)
					Transfer			(addon->ID());
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
	parent_addon(parent->cast<CAddon*>()),
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
	tmp.printf							("steps_%d", idx);
	steps								= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 1);
	step								= length / (float(steps) + .5f);

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
	
	tmp.printf							("icon_length_%d", idx);
	float icon_length					= READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.f);
	icon_step						= icon_length / (float(steps) + .5f);

	tmp.printf							("blocking_ironsights_%d", idx);
	blocking_iron_sights				= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 0);

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

void CAddonSlot::append_bone_trans(Fmatrix& trans, IKinematics* model, u16 bone, Fmatrix CR$ parent_trans) const
{
	trans.mul							(parent_trans, model->LL_GetTransform(bone));
}

void CAddonSlot::updateAddonsHudTransform(IKinematics* model, Fmatrix CR$ parent_trans)
{
	auto update_impl = [this, model, parent_trans](CAddon* addon, u16 bone_id_)
	{
		Fmatrix							trans;
		append_bone_trans				(trans, model, bone_id_, parent_trans);
		addon->updateHudTransform		(trans);
	};

	for (auto addon : addons)
		update_impl						(addon, bone_id);

	if (loading_addon)
		update_impl						(loading_addon, loading_bone_id);
}

void CAddonSlot::RenderHud() const
{
	for (auto addon : addons)
		addon->RenderHud				();
	
	if (loading_addon)
		loading_addon->RenderHud		();
}

void CAddonSlot::RenderWorld(IRenderVisual* model, Fmatrix CR$ parent_trans) const
{
	auto render_impl = [this, model, parent_trans](CAddon* addon, u16 bone_id_)
	{
		Fmatrix							trans;
		append_bone_trans				(trans, model->dcast_PKinematics(), bone_id_, parent_trans);
		addon->RenderWorld				(trans);
	};

	for (auto addon : addons)
		render_impl						(addon, bone_id);
	if (loading_addon)
		render_impl						(loading_addon, loading_bone_id);
}

int CAddonSlot::get_spacing(CAddon CPC left, CAddon CPC right) const
{
	if (step == 0.f)
		return							0;
	if (!left)
		return							0;
	if (!right)
		return							left->getLength(step);
	if (left->isLowProfile() || right->isLowProfile())
		return							left->getLength(step);
	return								left->getLength(step, CAddon::ProfileFwd) + right->getLength(step, CAddon::ProfileBwd);
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

	if (addon->getPos() == -1)
	{
		if (addon->isFrontPositioning())
		{
			auto I						= addons.end();
			int pos						= steps - get_spacing(addon, NULL);
			while (auto prev = get_prev_addon(I))
			{
				if (pos >= prev->getPos() + get_spacing(prev, addon))
				{
					I++;
					break;
				}
				pos						= prev->getPos() - get_spacing(addon, prev);
			}
			addons.insert				(I, addon);
			addon->setPos				(pos);
		}
		else
		{
			auto I						= --addons.begin();
			int pos						= get_spacing(NULL, addon);
			while (auto next = get_next_addon(I))
			{
				if (pos + get_spacing(addon, next) <= next->getPos())
					break;
				pos						= next->getPos() + get_spacing(next, addon);
			}
			addons.insert				(I, addon);
			addon->setPos				(pos);
		}
	}
	else
	{
		auto I							= addons.begin();
		while (auto next = get_next_addon(I))
			if (addon->getPos() < next->getPos())
				break;
		addons.insert					(I, addon);
	}

	updateAddonLocalTransform			(addon);
}

void CAddonSlot::attachLoadingAddon(CAddon* addon)
{
	loading_addon						= addon;
	Fmatrix trans						= Fmatrix().mul(loading_bone_offset, loading_model_offset);
	loading_addon->updateLocalTransform	(trans);
	loading_addon->setRootBoneID		(loading_bone_id);
}

void CAddonSlot::updateAddonLocalTransform(CAddon* addon) const
{
	Fmatrix								transform;
	transform.mul						(bone_offset, model_offset);
	transform.c.z						+= float(addon->getPos()) * step;
	if (parent_addon)
	{
		addon->updateLocalTransform		(Fmatrix().mul(parent_addon->getLocalTransform(), transform));
		addon->setRootBoneID			(parent_addon->getRootBoneID());
	}
	else
	{
		addon->updateLocalTransform		(transform);
		addon->setRootBoneID			(bone_id);
	}
}

void CAddonSlot::detachAddon(CAddon* addon)
{
	addon->setSlot						(NULL);
	addon->setSlotIdx					(-1);
	addon->setPos						(-1);
	addons.erase						(::std::find(addons.begin(), addons.end(), addon));
}

void CAddonSlot::detachLoadingAddon()
{
	loading_addon						= NULL;
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

	if (overlaping_slot != u16_max && parent_ao->AddonSlots()[overlaping_slot]->addons.size())
		return							false;

	return								true;
}

bool CAddonSlot::CanTake(CAddon CPC addon) const
{
	if (!Compatible(addon))
		return							false;
	if (magazine)
		return							true;

	int pos								= get_spacing(NULL, addon);
	for (auto next : addons)
	{
		if (pos + get_spacing(addon, next) <= next->getPos())
			return						true;
		pos								= next->getPos() + get_spacing(next, addon);
	}
	
	return								(pos + get_spacing(addon, NULL) <= steps);
}

void CAddonSlot::shiftAddon(CAddon* addon, int shift)
{
	auto A								= ::std::find(addons.begin(), addons.end(), addon);
	auto I								= A;
	int pos								= addon->getPos();
	int prev_pos						= pos;
	if (shift > 0)
	{
		auto next						= get_next_addon(I);
		while (shift--)
		{
			int npos					= pos + 1;
			int lim_pos					= (next) ? next->getPos() : steps;
			if (npos + get_spacing(addon, next) <= lim_pos)
				pos						= npos;
			else while (next)
			{
				npos					= next->getPos() + get_spacing(next, addon);
				next					= get_next_addon(I);
				lim_pos					= (next) ? next->getPos() : steps;
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
			int lim_pos					= (prev) ? prev->getPos(): 0;
			if (npos >= lim_pos + get_spacing(prev, addon))
				pos						= npos;
			else while (prev)
			{
				npos					= prev->getPos() - get_spacing(addon, prev);
				prev					= get_prev_addon(I);
				lim_pos					= (prev) ? prev->getPos() : 0;
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

	if (pos != addon->getPos())
	{
		addon->setPos					(pos);
		updateAddonLocalTransform		(addon);
	}
}
