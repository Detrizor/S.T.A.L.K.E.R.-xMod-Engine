#include "stdafx.h"
#include "Magazine.h"
#include "Level.h"
#include "player_hud.h"
#include "../Include/xrRender/Kinematics.h"
#include "WeaponAmmo.h"
#include "Weapon.h"

MMagazine::MMagazine(CGameObject* obj, shared_str CR$ section) :
	CModule(obj),
	m_capacity(pSettings->r_u32(section, "capacity")),
	m_bullets_visible(!!pSettings->r_bool(section, "bullets_visible")),
	m_attach_anm(pSettings->r_string(section, "attach_anm")),
	m_detach_anm(pSettings->r_string(section, "detach_anm"))
{
	LPCSTR S							= pSettings->r_string(section, "ammo_class");
	string128							_ammoItem;
	int count							= _GetItemCount(S);
	for (int it = 0; it < count; ++it)
	{
		_GetItem						(S, it, _ammoItem);
		m_ammo_types.push_back			(_ammoItem);
	}
	
	update_bullets_visibility			();
}

shared_str bullets_str					= "bullets";
void MMagazine::update_bullets_visibility()
{
	if (!m_bullets_visible)
		return;

	bool vis							= !Empty();
	O.UpdateBoneVisibility				(bullets_str, vis);
	I->SetInvIconType					(static_cast<u8>(vis));
	update_hud_bullets_visibility		();
}

void MMagazine::update_hud_bullets_visibility()
{
	if (auto hi = O.scast<CHudItem*>()->HudItemData())
		hi->set_bone_visible			(bullets_str, (BOOL)!Empty(), TRUE);
}

float MMagazine::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eUpdateHudBonesVisibility:
			update_hud_bullets_visibility();
			break;
		case eOnChild:
		{
			CWeaponAmmo* heap			= static_cast<CObject*>(data)->scast<CWeaponAmmo*>();
			if (!heap)
				break;

			if (param)
			{
				u8& idx					= heap->m_mag_pos;
				if (idx == u8_max)
					idx					= m_heaps.size();
				else
				{
					m_amount			+= heap->GetAmmoCount();
					m_weight			+= heap->Weight();
				}
				if (m_heaps.size() <= idx)
					m_heaps.resize		(idx + 1, nullptr);
				m_heaps[idx]			= heap;
			}
			else
			{
				m_heaps.erase_data		(heap);
				m_amount				-= heap->GetAmmoCount();
				m_weight				-= heap->Weight();
			}

			if (!O.scast<CWeapon*>())
				update_bullets_visibility();

			break;
		}

		case eWeight:
		{
			if (m_weight == flt_max)
			{
				m_weight				= 0.f;
				for (auto heap : m_heaps)
					m_weight			+= heap->Weight();
			}
			return						m_weight;
		}
		case eGetAmount:
			return						static_cast<float>(Amount());
		case eGetBar:
		case eGetFill:
		{
			float fill					= static_cast<float>(Amount()) / static_cast<float>(Capacity());
			if (type == eGetBar)
				return					(fLess(fill, 1.f)) ? fill : -1.f;
			return						fill;
		}
	}

	return								CModule::aboba(type, data, param);
}

bool MMagazine::CanTake(CWeaponAmmo CPC ammo)
{
	if (Full())
		return							false;

	for (auto& I : m_ammo_types)
	{
		if (I == ammo->m_section_id)
			return						true;
	}

	return								false;
}

void MMagazine::loadCartridge(CCartridge CR$ cartridge, int count)
{
	m_amount							+= count;
	R_ASSERT							(m_amount <= m_capacity);
	m_weight							+= cartridge.Weight();

	CWeaponAmmo* back_heap				= (m_heaps.empty()) ? nullptr : m_heaps.back();
	if (back_heap && back_heap->cNameSect() == cartridge.m_ammoSect && fEqual(back_heap->GetCondition(), cartridge.m_fCondition))
	{
		int d_count						= min(count, back_heap->m_boxSize - back_heap->GetAmmoCount());
		back_heap->ChangeAmmoCount		(d_count);
		count							-= d_count;
	}
	if (count)
		O.giveItems						(cartridge.m_ammoSect.c_str(), count, cartridge.m_fCondition);
}

void MMagazine::loadCartridge(CWeaponAmmo* ammo)
{
	CCartridge							cartridge;
	ammo->Get							(cartridge);
	loadCartridge						(cartridge);
}

bool MMagazine::getCartridge(CCartridge& destination, bool expend)
{
	if (Empty())
		return							false;

	bool res							= m_heaps.back()->Get(destination, expend);
	if (res && expend)
	{
		--m_amount;
		m_weight						-= destination.Weight();
	}

	return								res;
}

void MMagazine::setCondition(float val)
{
	for (auto h : m_heaps)
		h->getModule<CInventoryItem>()->SetCondition(val);
}
