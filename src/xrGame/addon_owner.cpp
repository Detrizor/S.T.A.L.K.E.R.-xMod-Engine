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
#include "weaponBM16.h"
#include "ui/UICellCustomItems.h"
#include "item_usable.h"

bool MAddonOwner::loadAddonSlots(shared_str CR$ section, VSlots& slots, MAddonOwner* ao)
{
	shared_str							slots_section;
	if (pSettings->line_exist(section, "slots"))
		slots_section					= pSettings->r_string(section, "slots");
	else
		slots_section.printf			("slots_%s", *section);

	shared_str							tmp;
	u16 i								= 0;
	while (pSettings->line_exist(slots_section, tmp.printf("type_%d", i)))
		slots.emplace_back(i++, ao)->load(slots_section, section);

	return								pSettings->r_bool_ex(slots_section, "base_foreground_draw", false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MAddonOwner::MAddonOwner(CGameObject* obj) : CModule(obj)
{
	m_base_foreground_draw				= loadAddonSlots(O.cNameSect(), m_slots, this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAddonOwner::sOnChild(CGameObject* obj, bool take)
{
	if (auto addon = obj->getModule<MAddon>())
	{
		if (take && !addon->getSlot())
		{
			if (addon->getSlotStatus() == MAddon::attached)
			{
				if (addon->getSlotIdx() < m_slots.size() && m_slots[addon->getSlotIdx()]->canTake(addon))
					finishAttaching		(addon, m_slots[addon->getSlotIdx()].get());
				else
					addon->onDetach		();
			}
			else if (addon->getSlotStatus() == MAddon::need_to_attach)
			{
				if (auto slot = find_available_slot(addon))
					slot->parent_ao->finishAttaching(addon, slot);
				else
					addon->onDetach		();
			}
		}
		else if (!take && addon->getSlot() && addon->getSlot()->parent_ao == this)
			finishDetaching				(addon, false);
	}
}

void MAddonOwner::sRenderableRender()
{
	for (auto& s : m_slots)
		if (s->getIconDraw())
			s->RenderWorld				(O.XFORM());
}

void MAddonOwner::sUpdateSlotsTransform()
{
	attachable_hud_item* hi				= O.scast<CHudItem*>()->HudItemData();
	for (auto& s : m_slots)
		s->updateAddonsHudTransform		(hi);
}

void MAddonOwner::sRenderHudMode()
{
	for (auto& s : m_slots)
		s->RenderHud					();
}

float MAddonOwner::sSumItemData(EItemDataTypes type)
{
	float res							= 0.f;
	for (auto& slot : m_slots)
		for (auto addon : slot->addons)
			res							+= addon->I->getData(type);
	return								res;
}

xoptional<CUICellItem*> MAddonOwner::sCreateIcon()
{
	return								xr_new<CUIAddonOwnerCellItem>(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAddonOwner::register_addon(MAddon PC$ addon, bool attach) const
{
	if (addon->getSlot()->getIconDraw())
		O.scast<CInventoryItem*>()->invalidateIcon();

	process_addon						(addon, attach);
	if (auto self_addon = O.getModule<MAddon>())
		if (auto slot = self_addon->getSlot())
			if (auto parent_ao = slot->parent_ao)
				parent_ao->register_addon(addon, attach);
}

void MAddonOwner::process_addon(MAddon PC$ addon, bool attach, bool recurrent) const
{
	if (attach)
		O.emitSignal					(sOnAddon(addon, (recurrent) ? 2 : 1));

	if (auto addon_ao = addon->O.getModule<MAddonOwner>())
		for (auto& s : addon_ao->AddonSlots())
			for (auto a : s->addons)
				process_addon			(a, attach, true);

	if (!attach)
		O.emitSignal					(sOnAddon(addon, (recurrent) ? -1 : 0));
}

bool MAddonOwner::tryAttach(MAddon* addon, bool forced) const
{
	if (auto slot = find_available_slot(addon, forced))
	{
		if (O.scast<CWeaponBM16*>())
			if (auto ammo = addon->O.scast<CWeaponAmmo*>())
				if (ammo->GetAmmoCount() >= 2)
					slot				= nullptr;
		addon->startAttaching			(slot);
		return							true;
	}

	return								false;
}

void MAddonOwner::finishAttaching(MAddon* addon, CAddonSlot* slot) const
{
	if (!slot)
		slot							= m_slots[addon->getSlotIdx()].get();
	if (!slot->steps && slot->addons.size())
		finishDetaching					(slot->addons.front());
	slot->attachAddon					(addon);
	register_addon						(addon, true);
	addon->O.transfer					(O.ID());
}

void MAddonOwner::finishDetaching(MAddon* addon, bool transfer) const
{
	register_addon						(addon, false);
	addon->getSlot()->detachAddon		(addon, static_cast<int>(transfer));
}

CAddonSlot* MAddonOwner::find_available_slot(MAddon CPC addon, bool forced) const
{
	for (auto& s : m_slots)
	{
		if (s->canTake(addon, forced))
			return						s.get();
		for (auto a : s->addons)
			if (auto ao = a->O.getModule<MAddonOwner>())
				if (auto slot = ao->find_available_slot(addon, forced))
					return				slot;
	}
	return								nullptr;
}

void MAddonOwner::calcSlotsBoneOffset(attachable_hud_item* hi)
{
	m_root_offset						= static_cast<Dmatrix>(hi->m_model->LL_GetTransform_R(0));
	for (auto& s : m_slots)
		s->calcBoneOffset				(hi);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LPCSTR get_base_slot_name(LPCSTR slot_type)
{
	return								CStringTable().translate(shared_str().printf("st_addon_slot_%s", slot_type)).c_str();
}

LPCSTR CAddonSlot::getSlotName(LPCSTR slot_type)
{
	auto process_exceptions = [slot_type](RStringVec& vec, LPCSTR divider)
	{
		shared_str res					= 0;
		for (auto& ex : vec)
		{
			LPCSTR slot_name			= get_base_slot_name(ex.c_str());
			if (!res)
				res						= slot_name;
			else
				res.printf				("%s%s%s", res.c_str(), divider, slot_name);
		}
		return							res.c_str();
	};

	if (slot_exceptions.contains(slot_type))
		return							process_exceptions(slot_exceptions[slot_type], " / ");
	else if (addon_exceptions.contains(slot_type))
		return							process_exceptions(addon_exceptions[slot_type], ", ");
	return								get_base_slot_name(slot_type);
}

CAddonSlot::CAddonSlot(u16 _idx, MAddonOwner PC$ _parent_ao) : parent_ao(_parent_ao), idx(_idx)
{
}

void CAddonSlot::load(shared_str CR$ section, shared_str CR$ parent_section)
{
	shared_str							tmp;

	tmp.printf							("type_%d", idx);
	pSettings->w_string_ex				(type, section, tmp);
	name								= getSlotName(type.c_str());

	tmp.printf							("length_%d", idx);
	double length						= static_cast<double>(pSettings->r_double_ex(section, tmp, 0.));

	tmp.printf							("step_%d", idx);
	if (pSettings->w_double_ex(step, section, tmp))
		steps							= static_cast<int>(round(length / step));

	tmp.printf							("model_offset_rot_%d", idx);
	Fvector model_rot					= vZero;
	if (pSettings->w_fvector3d2r_ex(model_rot, section, tmp))
		model_offset.setHPBv			(static_cast<Dvector>(model_rot));

	tmp.printf							("model_offset_pos_%d", idx);
	pSettings->w_dvector_ex				(model_offset.c, section, tmp);

	tmp.printf							("name_suffix_%d", idx);
	shared_str suffix					= 0;
	if (!pSettings->w_string_ex(suffix, section, tmp))
	{
		if (abs(model_rot.z) >= PI * .75f)
			suffix						= "st_bottom";
		else if (model_rot.z <= -PI_DIV_4)
			suffix						= "st_left";
		else if (model_rot.z >= PI_DIV_4)
			suffix						= "st_right";
	}
	if (!!suffix)
		name.printf						("%s (%s)", name.c_str(), CStringTable().translate(suffix).c_str());

	Fvector2 icon_origin				= pSettings->r_fvector2(parent_section, "inv_icon_origin");
	float icon_ppm						= pSettings->r_float(parent_section, "icon_ppm");
	bool icon_inversed					= READ_IF_EXISTS(pSettings, r_bool, parent_section, "icon_inversed", false);

	icon_offset							= icon_origin;
	float x_offset						= static_cast<float>(model_offset.c.z) * icon_ppm;
	float y_offset						= static_cast<float>(model_offset.c.y) * icon_ppm;
	icon_offset.x						-= x_offset;
	icon_offset.y						-= (icon_inversed) ? -y_offset : y_offset;
	icon_step							= static_cast<float>(length) * icon_ppm / (static_cast<float>(steps) + .5f);

	tmp.printf							("overlapping_slot_%d", idx);
	pSettings->w_u16_ex					(m_overlaping_slot, section, tmp);
	
	tmp.printf							("attach_bone_%d", idx);
	pSettings->w_string_ex				(m_attach_bone, section, tmp);
	
	tmp.printf							("draw_icon_%d", idx);
	m_icon_draw							= pSettings->r_bool_ex(section, tmp, !m_attach_bone || !strstr(m_attach_bone.c_str(), "chamber"));
	
	tmp.printf							("background_draw_%d", idx);
	pSettings->w_bool_ex				(m_background_draw, section, tmp);
	m_background_draw					|= (m_attach_bone == "magazine" || m_attach_bone == "grenade");
	
	tmp.printf							("foreground_draw_%d", idx);
	pSettings->w_bool_ex				(m_foreground_draw, section, tmp);
}

int CAddonSlot::get_spacing(MAddon CPC left, MAddon CPC right) const
{
	if (step == 0.)
		return							0;
	if (!left)
		return							-1;
	if (!right)
		return							left->getLength(step) - 1;
	if (left->isLowProfile() || right->isLowProfile())
		return							left->getLength(step);

	int spacing_profile					= left->getLength(step, MAddon::ProfileFwd) + right->getLength(step, MAddon::ProfileBwd);
	return								max(left->getLength(step), spacing_profile);
}

MAddon* CAddonSlot::get_next_addon(xr_list<MAddon*>::iterator& I) const
{
	return								(++I == addons.end()) ? NULL : *I;
}

MAddon* CAddonSlot::get_prev_addon(xr_list<MAddon*>::iterator& I) const
{
	return								(I == addons.begin()) ? NULL : *--I;
}

void CAddonSlot::update_addon_local_transform(MAddon* addon) const
{
	auto parent_addon					= parent_ao->O.getModule<MAddon>();
	updateAddonLocalTransform			(addon, (parent_addon) ? & parent_addon->getLocalTransform() : nullptr);
}

void CAddonSlot::attachAddon(MAddon* addon)
{
	addon->onAttach						(this);

	if (addon->getSlotPos() == MAddon::no_idx)
	{
		if (addon->isFrontPositioning())
		{
			auto I						= addons.end();
			int first_pos				= steps - get_spacing(addon, nullptr) + 1;
			int pos						= first_pos;
			while (auto prev = get_prev_addon(I))
			{
				if (pos == first_pos && (pos < prev->getSlotPos() + get_spacing(prev, addon)))
					pos					+= 1;
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
			int pos						= 0;
			while (auto next = get_next_addon(I))
			{
				if (pos == 0 && (get_spacing(addon, next) > next->getSlotPos()))
					pos					= -1;
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

	update_addon_local_transform		(addon);
}

void CAddonSlot::detachAddon(MAddon* addon, int transfer)
{
	addon->onDetach						(transfer);
	addons.erase						(_STD find(addons.begin(), addons.end(), addon));
	update_addon_local_transform		(addon);
}

#include "../../../xrEngine/xr_input.h"
void CAddonSlot::shiftAddon(MAddon* addon, int shift)
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
		update_addon_local_transform	(addon);
		parent_ao->O.scast<CInventoryItem*>()->invalidateIcon();
	}
}

void CAddonSlot::updateAddonsHudTransform(attachable_hud_item* hi)
{
	static Dmatrix						bone_trans;
	bone_trans.mul						(hi->m_transform, static_cast<Dmatrix>(hi->m_model->LL_GetTransform(m_attach_bone_id)));
	for (auto addon : addons)
	{
		addon->updateHudTransform		(bone_trans);
		if (addon->slots)
			for (auto& s : *addon->slots)
				s->updateAddonsHudTransform(hi);
	}
}

void CAddonSlot::startReloading(MAddon* loading_addon)
{
	m_loading_addon						= loading_addon;
}

void CAddonSlot::loadingDetach()
{
	if (parent_ao->O.H_Root()->scast<CActor*>())
		while (addons.size())
			parent_ao->finishDetaching	(addons.back());
}

void CAddonSlot::loadingAttach()
{
	if (m_loading_addon && parent_ao->O.H_Root()->scast<CActor*>())
		parent_ao->finishAttaching		(m_loading_addon, this);
}

void CAddonSlot::finishLoading(bool interrupted)
{
	if (interrupted && parent_ao->O.H_Root()->scast<CActor*>())
		while (addons.size())
			parent_ao->finishDetaching	(addons.back());
	if (m_loading_addon)
		m_loading_addon					= nullptr;
}

void CAddonSlot::RenderHud() const
{
	for (auto addon : addons)
	{
		addon->RenderHud				();
		if (auto ao = addon->O.getModule<MAddonOwner>())
			for (auto& s : ao->AddonSlots())
				s->RenderHud			();
	}
}

void CAddonSlot::RenderWorld(Fmatrix CR$ parent_trans) const
{
	for (auto addon : addons)
	{
		addon->RenderWorld				(parent_trans);
		if (auto ao = addon->O.getModule<MAddonOwner>())
			for (auto& s : ao->AddonSlots())
				s->RenderWorld			(parent_trans);
	}
}

bool CAddonSlot::isCompatible(shared_str CR$ slot_type, shared_str CR$ addon_type)
{
	if (slot_type.size() && addon_type == slot_type)
		return							true;

	if (slot_exceptions.contains(slot_type.c_str()))
		return							slot_exceptions[slot_type.c_str()].contains(addon_type);
	
	if (addon_exceptions.contains(addon_type.c_str()))
		return							addon_exceptions[addon_type.c_str()].contains(slot_type);

	return								false;
}

bool CAddonSlot::canTake(MAddon CPC addon, bool forced) const
{
	if (!forced && parent_ao)
		if (auto hi = parent_ao->O.scast<CHudItem*>())
			if (hi->HudItemData())
				if (hi->IsPending())
					return				false;

	if (!isCompatible(type, addon->SlotType()))
		return							false;
	if (m_overlaping_slot != u16_max && parent_ao && parent_ao->AddonSlots()[m_overlaping_slot]->addons.size())
		return							false;
	if (!steps)
		return							(addons.empty() || forced);

	int pos								= get_spacing(nullptr, addon);
	for (auto next : addons)
	{
		if (pos + get_spacing(addon, next) <= next->getSlotPos())
			return						true;
		pos								= next->getSlotPos() + get_spacing(next, addon);
	}
	
	return								(pos + get_spacing(addon, nullptr) <= steps);
}

void CAddonSlot::updateAddonLocalTransform(MAddon* addon, Dmatrix CPC parent_transform) const
{
	if (addon->getSlot() == this)
	{
		Dmatrix transform				= model_offset;
		transform.c.z					+= static_cast<double>(addon->getSlotPos()) * step;
		if (parent_transform)
			transform.mulA_43			(*parent_transform);
		addon->setLocalTransform		(transform);
		addon->updateHudOffset			(m_attach_bone_offset, parent_ao->getRootOffset());
	}
	else
		addon->setLocalTransform		(Didentity);

	if (addon->slots)
		for (auto& s : *addon->slots)
			for (auto a : s->addons)
				s->updateAddonLocalTransform(a, &addon->getLocalTransform());
}

void CAddonSlot::calcBoneOffset(attachable_hud_item* hi)
{
	if (!!m_attach_bone)
	{
		shared_str						line;
		line.printf						("%s_bone_name", m_attach_bone.c_str());
		if (pSettings->line_exist(hi->m_hud_section, line))
		{
			LPCSTR bone_name			= pSettings->r_string(hi->m_hud_section, line.c_str());
			m_attach_bone_id			= hi->m_model->LL_BoneID(bone_name);
		}
	}
	m_attach_bone_offset				= static_cast<Dmatrix>(hi->m_model->LL_GetTransform(m_attach_bone_id));

	for (auto a : addons)
	{
		a->updateHudOffset				(m_attach_bone_offset, parent_ao->getRootOffset());
		if (auto ao = a->O.getModule<MAddonOwner>())
			ao->calcSlotsBoneOffset		(hi);
}
	}

CAddonSlot::exceptions_list CAddonSlot::slot_exceptions{};
CAddonSlot::exceptions_list CAddonSlot::addon_exceptions{};

void CAddonSlot::loadStaticData()
{
	auto load_exceptions_list = [](LPCSTR type, exceptions_list& dest)
	{
		auto& section					= pSettings->r_section(type);
		for (auto& line : section.Data)
		{
			LPCSTR str					= line.second.c_str();
			string128					item;
			RStringVec vec				= {};
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				_GetItem				(str, i, item);
				vec.push_back			(item);
			}
			dest[line.first.c_str()]	= _STD move(vec);
		}
	};
	load_exceptions_list				("slot_type_exceptions", slot_exceptions);
	load_exceptions_list				("addon_type_exceptions", addon_exceptions);
}
