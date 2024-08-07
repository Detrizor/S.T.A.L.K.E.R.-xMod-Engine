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
	LoadAddonSlots						(O.cNameSect(), m_slots, this);
}

void CAddonOwner::LoadAddonSlots(shared_str CR$ section, VSlots& slots, CAddonOwner PC$ parent_ao)
{
	shared_str							slots_section;
	if (pSettings->line_exist(section, "slots"))
		slots_section					= pSettings->r_string(section, "slots");
	else
		slots_section.printf			("slots_%s", *section);

	shared_str							tmp;
	u16 i								= 0;
	while (pSettings->line_exist(slots_section, tmp.printf("type_%d", i)))
		slots.push_back					(xr_new<CAddonSlot>(slots_section, i++, parent_ao));
}

void CAddonOwner::calculateSlotsBoneOffset(IKinematics* model, shared_str CR$ hud_sect)
{
	for (auto& s : m_slots)
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
			for (auto& s : m_slots)
				s->RenderWorld			(O.XFORM());
			break;
		}
		case eOnChild:
			if (auto addon = cast<CAddon*>((CObject*)data))
			{
				auto slot				= addon->getSlot();
				if (slot)
				{
					if (param && slot->parent_ao == this || !param && slot->parent_ao != this)
						return			CModule::aboba(type, data, param);
				}
				else if (param && addon->getSlotIdx() != u16_max)
					slot				= m_slots[addon->getSlotIdx()];

				if (!slot)
				{
					if (param && (slot = findAvailableSlot(addon)))
					{
						addon->setSlotIdx(slot->idx);
						if (slot->parent_ao != this)
							addon->Transfer(slot->parent_ao->O.ID());
					}
					
					if (!slot)
					{
						if (param)
							transfer_addon(addon, false);
						return			CModule::aboba(type, data, param);
					}
				}

				if (param)
					slot->attachAddon	(addon);
				slot->parent_ao->RegisterAddon(addon, param);
				if (!param)
					slot->detachAddon	(addon);
			}
			break;
		case eWeight:
		case eVolume:
		case eCost:
		{
			float res					= 0.f;
			for (auto& slot : m_slots)
				for (auto addon : slot->addons)
					res					+= addon->O.Aboba(type);
			return						res;
		}
		case eRenderHudMode:
			for (auto& s : m_slots)
				s->RenderHud			();
			break;
		case eUpdateSlotsTransform:
		{
			attachable_hud_item* hi		= O.Cast<CHudItem*>()->HudItemData();
			Dmatrix trans				= hi->m_transform;
			for (auto& s : m_slots)
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
		for (auto& s : addon_ao->AddonSlots())
			for (auto a : s->addons)
				processAddon			(a, attach);

	if (!attach)
		O.Aboba							(eOnAddon, (void*)addon, false);
}

CAddonSlot* CAddonOwner::findAvailableSlot(CAddon CPC addon) const
{
	for (auto& s : m_slots)
	{
		if (s->CanTake(addon))
			return						s.get();
		for (auto a : s->addons)
			if (auto ao = a->cast<CAddonOwner*>())
				if (auto slot = ao->findAvailableSlot(addon))
					return				slot;
	}
	return								nullptr;
}

bool CAddonOwner::attachAddon(CAddon* addon)
{
	CAddonSlot* slot					= nullptr;
	if (addon->getSlotIdx() == u16_max)
	{
		if (slot = findAvailableSlot(addon))
			addon->setSlotIdx			(slot->idx);
	}
	else
		slot							= m_slots[addon->getSlotIdx()];

	if (slot)
	{
		slot->parent_ao->transfer_addon	(addon, true);
		return							true;
	}

	return								false;
}

void CAddonOwner::detachAddon(CAddon* addon)
{
	transfer_addon						(addon, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LPCSTR CAddonSlot::getSlotName(LPCSTR slot_type)
{
	return								CStringTable().translate(shared_str().printf("st_addon_slot_%s", slot_type)).c_str();
}

CAddonSlot::CAddonSlot(shared_str CR$ section, u16 _idx, CAddonOwner PC$ parent) :
	parent_ao(parent),
	idx(_idx)
{
	shared_str							tmp;

	tmp.printf							("type_%d", idx);
	type								= pSettings->r_string(section, *tmp);

	tmp.printf							("length_%d", idx);
	double length						= static_cast<double>(READ_IF_EXISTS(pSettings, r_float, section, *tmp, 0.));

	tmp.printf							("step_%d", idx);
	if (pSettings->line_exist(section, tmp))
	{
		m_step							= static_cast<double>(pSettings->r_float(section, *tmp));
		steps							= static_cast<int>(round(length / m_step));
	}
	else
	{
		tmp.printf						("steps_%d", idx);
		steps							= READ_IF_EXISTS(pSettings, r_s32, section, *tmp, 1);
		m_step							= length / (static_cast<double>(steps) + .5);
	}

	tmp.printf							("model_offset_rot_%d", idx);
	Fvector model_rot					= READ_IF_EXISTS(pSettings, r_fvector3d2r, section, *tmp, vZero);
	model_offset.setHPBv				(static_cast<Dvector>(model_rot));
	tmp.printf							("model_offset_pos_%d", idx);
	model_offset.translate_over			(static_cast<Dvector>(READ_IF_EXISTS(pSettings, r_fvector3, section, *tmp, vZero)));

	name								= getSlotName(*type);
	tmp.printf							("name_suffix_%d", idx);
	shared_str suffix					= READ_IF_EXISTS(pSettings, r_string, section, *tmp, "");
	if (!suffix.size())
	{
		if (abs(model_rot.z) >= PI * .75f)
			suffix						= "st_bottom";
		else if (model_rot.z <= -PI_DIV_4)
			suffix						= "st_left";
		else if (model_rot.z >= PI_DIV_4)
			suffix						= "st_right";
	}
	if (suffix.size())
		name.printf						("%s (%s)", *name, *CStringTable().translate(suffix));

	Fvector2 icon_origin				= pSettings->r_fvector2(parent->O.cNameSect(), "inv_icon_origin");
	float icon_ppm						= pSettings->r_float(parent->O.cNameSect(), "icon_ppm");
	icon_offset.x						= icon_origin.x - static_cast<float>(model_offset.c.z) * icon_ppm;
	icon_offset.y						= icon_origin.y - static_cast<float>(model_offset.c.y) * icon_ppm;
	icon_step							= static_cast<float>(length) * icon_ppm / (static_cast<float>(steps) + .5f);

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
	updateAddonLocalTransform			(addon);
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
		trans.translate_mul				(parent_ao->O.getRootBonePosition().mul(-1.));
		addon->updateHudTransform		(trans);
		if (auto ao = addon->cast<CAddonOwner*>())
			for (auto& s : ao->AddonSlots())
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
			for (auto& s : ao->AddonSlots())
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
			for (auto& s : ao->AddonSlots())
				s->RenderWorld			(parent_trans);
	}
}

bool CAddonSlot::isCompatible(shared_str CR$ slot_type, shared_str CR$ addon_type)
{
	if (addon_type == slot_type)
		return							true;

	if (pSettings->line_exist("slot_type_exceptions", slot_type))
		return							!!strstr(pSettings->r_string("slot_type_exceptions", *slot_type), *addon_type);

	if (pSettings->line_exist("addon_type_exceptions", addon_type))
		return							!!strstr(pSettings->r_string("addon_type_exceptions", *addon_type), *slot_type);

	return								false;
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
	if (addon->getSlot() == this)
	{
		Dmatrix transform				= model_offset;
		transform.c.z					+= static_cast<double>(addon->getSlotPos()) * m_step;
		if (auto parent_addon = parent_ao->cast<CAddon*>())
			transform.mulA_43			(parent_addon->getLocalTransform());
		addon->setLocalTransform		(transform);
	}
	else
		addon->setLocalTransform		(Didentity);

	if (auto ao = addon->cast<CAddonOwner*>())
		for (auto& s : ao->AddonSlots())
			for (auto a : s->addons)
				s->updateAddonLocalTransform(a);
}
