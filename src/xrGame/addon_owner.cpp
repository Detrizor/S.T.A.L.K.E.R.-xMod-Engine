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
#include "item_usable.h"

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

void CAddonOwner::calculateSlotsBoneOffset(IKinematics* model, shared_str CR$ hud_sect)
{
	for (auto s : m_Slots)
		s->calculateBoneOffset			(model, hud_sect);
}

void CAddonOwner::transfer_addon(CAddon CPC addon, bool attach)
{
	if (O.Aboba(eTransferAddon, (void*)addon, (int)attach) == flt_max)
	{
		if (attach)
			addon->Transfer				(O.ID());
		else
		{
			auto root					= O.H_Root();
			addon->Transfer				((root->Cast<CInventoryOwner*>()) ? root->ID() : u16_max);
		}
	}
}

float CAddonOwner::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eRenderableRender:
		{
			for (auto s : m_Slots)
				s->RenderWorld			(O.XFORM());
			break;
		}
		case eOnChild:
			if (auto addon = cast<CAddon*>((CObject*)data))
			{
				auto slot				= addon->getSlot();
				if (!slot && param && addon->getSlotIdx() != u16_max)
					slot				= m_Slots[addon->getSlotIdx()];
				if (!slot)
				{
					if (param && (slot = findAvailableSlot(addon)))
						addon->setSlotIdx(slot->idx);
					
					if (!slot)
					{
						if (param)
							for (auto s : m_Slots)
								for (auto a : s->addons)
									if (auto ao = a->cast<CAddonOwner*>())
										if (ao->attachAddon(addon))
											return CModule::aboba(type, data, param);
						transfer_addon	(addon, false);
						return			CModule::aboba(type, data, param);
					}
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
			Dmatrix trans				= hi->m_transform;
			trans.translate_mul			(O.getRootBonePosition().mul(-1.));
			for (auto s : m_Slots)
				s->updateAddonsHudTransform(hi->m_model, trans);
			break;
		}
	}

	return								CModule::aboba(type, data, param);
}

CAddonOwner* CAddonOwner::getParentAO() const
{
	return								(O.H_Parent()) ? O.H_Parent()->Cast<CAddonOwner*>() : nullptr;
}

void CAddonOwner::RegisterAddon(CAddon PC$ addon, bool attach) const
{
	processAddon						(addon, attach);
	if (auto parent_ao = getParentAO())
		parent_ao->RegisterAddon		(addon, attach);
}

void CAddonOwner::processAddon(CAddon PC$ addon, bool attach) const
{
	if (attach)
		O.Aboba							(eOnAddon, (void*)addon, true);

	if (auto addon_ao = addon->cast<CAddonOwner*>())
		for (auto s : addon_ao->AddonSlots())
			for (auto a : s->addons)
				processAddon			(a, attach);

	if (!attach)
		O.Aboba							(eOnAddon, (void*)addon, false);
}

CAddonSlot* CAddonOwner::findAvailableSlot(CAddon CPC addon) const
{
	for (auto s : m_Slots)
	{
		if (s->CanTake(addon))
			return						s;
	}
	return								nullptr;
}

bool CAddonOwner::attachAddon(CAddon* addon)
{
	if (addon->getSlotIdx() == u16_max)
		if (auto slot = findAvailableSlot(addon))
			addon->setSlotIdx			(slot->idx);

	if (addon->getSlotIdx() != u16_max)
	{
		transfer_addon					(addon, true);
		return							true;
	}

	return								false;
}

void CAddonOwner::detachAddon(CAddon* addon)
{
	transfer_addon						(addon, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAddonSlot::CAddonSlot(LPCSTR section, u16 _idx, CAddonOwner PC$ parent) :
	parent_ao(parent),
	idx(_idx)
{
	shared_str							tmp;

	tmp.printf							("name_%d", idx);
	name								= CStringTable().translate(pSettings->r_string(section, *tmp));

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("length_%d", idx);
	double length						= static_cast<double>(READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.));

	tmp.printf							("step_%d", idx);
	if (pSettings->line_exist(section, tmp))
	{
		m_step							= static_cast<double>(pSettings->r_float(section, *tmp));
		steps							= (int)round(length / m_step);
	}
	else
	{
		tmp.printf						("steps_%d", idx);
		steps							= READ_IF_EXISTS(pSettings, r_s32, section, *tmp, 1);
		m_step							= length / (float(steps) + .5f);
	}

	tmp.printf							("model_offset_rot_%d", idx);
	model_offset.setHPBv				(static_cast<Dvector>(READ_IF_EXISTS(pSettings, r_fvector3d2r, section, *tmp, vZero)));
	tmp.printf							("model_offset_pos_%d", idx);
	model_offset.translate_over			(static_cast<Dvector>(READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero)));

	tmp.printf							("icon_offset_pos_%d", idx);
	icon_offset							= READ_IF_EXISTS(pSettings, r_fvector2, section, *tmp, vZero2);
	
	tmp.printf							("icon_length_%d", idx);
	float icon_length					= READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.f);
	icon_step							= icon_length / (float(steps) + .5f);

	tmp.printf							("blocking_ironsights_%d", idx);
	blocking_iron_sights				= READ_IF_EXISTS(pSettings, r_u8, section, *tmp, 0);

	tmp.printf							("overlapping_slot_%d", idx);
	m_overlaping_slot					= READ_IF_EXISTS(pSettings, r_u16, section, *tmp, u16_max);
	
	tmp.printf							("attach_%d", idx);
	attach								= READ_IF_EXISTS(pSettings, r_string, section, *tmp, 0);

	m_has_loading_anim					= attach == "magazine" || attach == "grenade";
	
	tmp.printf							("background_draw_%d", idx);
	m_background_draw					= !!READ_IF_EXISTS(pSettings, r_string, section, *tmp, FALSE);
}

void CAddonSlot::append_bone_trans(Dmatrix& trans, IKinematics* model) const
{
	if (m_bone_id != u16_max)
	{
		trans.mulB_43					(static_cast<Dmatrix>(model->LL_GetTransform(m_bone_id)));
		trans.mulB_43					(m_bone_offset);
	}
}

int CAddonSlot::get_spacing(CAddon CPC left, CAddon CPC right) const
{
	if (m_step == 0.f)
		return							0;
	int mount_edge_bonus				= (parent_ao->cast<CAddon*>()) ? -1 : 0;
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
		auto I							= --addons.begin();
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

#include "../../../xrEngine/xr_input.h"
void CAddonSlot::shiftAddon(CAddon* addon, int shift)
{
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
		shift							*= 64;
	if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
		shift							*= 16;
	if (pInput->iGetAsyncKeyState(DIK_LALT))
		shift							*= 4;

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

void CAddonSlot::updateAddonsHudTransform(IKinematics* model, Dmatrix CR$ parent_trans)
{
	for (auto addon : addons)
	{
		Dmatrix trans					= parent_trans;
		append_bone_trans				(trans, model);
		addon->updateHudTransform		(trans);
		if (auto ao = addon->cast<CAddonOwner*>())
			for (auto s : ao->AddonSlots())
				s->updateAddonsHudTransform(model, trans);
	}
}

void CAddonSlot::startReloading(CAddon* loading_addon)
{
	m_loading_addon						= loading_addon;
}

void CAddonSlot::loadingDetach()
{
	for (auto a : addons)
		a->Transfer						(parent_ao->O.H_Root()->ID());
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

void CAddonSlot::calculateBoneOffset(IKinematics* model, shared_str CR$ hud_sect)
{
	if (attach.size())
	{
		shared_str						line;
		line.printf						("%s_bone_name", *attach);
		if (pSettings->line_exist(hud_sect, line))
		{
			LPCSTR bone_name			= pSettings->r_string(hud_sect, *line);
			m_bone_id					= model->LL_BoneID(bone_name);
			m_bone_offset				= static_cast<Dmatrix>(model->LL_GetTransform(m_bone_id));
			m_bone_offset.invert		();
		}
	}

	for (auto a : addons)
		if (auto ao = a->cast<CAddonOwner*>())
			ao->calculateSlotsBoneOffset(model, hud_sect);
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

bool CAddonSlot::isCompatible(shared_str CR$ slot_type, shared_str CR$ addon_type)
{
	if (addon_type != slot_type)
	{
		if (!pSettings->line_exist("slot_exceptions", addon_type))
			return						false;
		
		if (strcmp(pSettings->r_string("slot_exceptions", *addon_type), *slot_type))
			return						false;
	}

	return								true;
}

bool CAddonSlot::CanTake(CAddon CPC addon) const
{
	if (!isCompatible(type, addon->SlotType()))
		return							false;
	if (m_overlaping_slot != u16_max && parent_ao->AddonSlots()[m_overlaping_slot]->addons.size())
		return							false;
	if (m_has_loading_anim)
		return							!isLoading();
	if (fIsZero(m_step))
		return							addons.empty();

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
	Dmatrix transform					= model_offset;
	transform.c.z						+= static_cast<double>(addon->getSlotPos()) * m_step;
	if (auto addon = parent_ao->cast<CAddon*>())
		transform.mulA_43				(addon->getLocalTransform());
	addon->setLocalTransform			(transform);

	if (auto ao = addon->cast<CAddonOwner*>())
		for (auto s : ao->AddonSlots())
			for (auto a : s->addons)
				s->updateAddonLocalTransform(a);
}
