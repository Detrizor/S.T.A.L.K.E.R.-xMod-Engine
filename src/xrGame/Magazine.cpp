#include "stdafx.h"
#include "Magazine.h"
#include "Level.h"
#include "player_hud.h"
#include "../Include/xrRender/Kinematics.h"
#include "WeaponAmmo.h"
#include "Weapon.h"

MMagazine::MMagazine(CGameObject* obj) : CModule(obj)
{
	m_Heaps.clear						();
	m_iNextHeapIdx						= u16_max;

	m_capacity							= pSettings->r_u32(O.cNameSect(), "capacity");
	LPCSTR S							= pSettings->r_string(O.cNameSect(), "ammo_class");
	string128							_ammoItem;
	int count							= _GetItemCount(S);
	m_ammo_types.clear					();
	for (int it = 0; it < count; ++it)
	{
		_GetItem						(S, it, _ammoItem);
		m_ammo_types.push_back			(_ammoItem);
	}
	m_bullets_visible					= !!pSettings->r_bool(O.cNameSect(), "bullets_visible");

	m_attach_anm						= pSettings->r_string(O.cNameSect(), "attach_anm");
	m_detach_anm						= pSettings->r_string(O.cNameSect(), "detach_anm");
	
	InvalidateState						();
	UpdateBulletsVisibility				();
}

void MMagazine::InvalidateState()
{
	m_SumAmount							= u16_max;
	m_SumWeight							= flt_max;
}

shared_str bullets_str					= "bullets";
void MMagazine::UpdateBulletsVisibility()
{
	if (!m_bullets_visible)
		return;

	bool vis							= !Empty();
	O.UpdateBoneVisibility				(bullets_str, vis);
	I->SetInvIconType					(static_cast<u8>(vis));
	UpdateHudBulletsVisibility			();
}

void MMagazine::UpdateHudBulletsVisibility()
{
	if (auto hi = O.scast<CHudItem*>()->HudItemData())
		hi->set_bone_visible			(bullets_str, (BOOL)!Empty(), TRUE);
}

float MMagazine::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eUpdateHudBonesVisibility:
			UpdateHudBulletsVisibility	();
			break;
		case eOnChild:
		{
			CWeaponAmmo* heap			= static_cast<CObject*>(data)->scast<CWeaponAmmo*>();
			if (!heap)
				break;

			if (param)
			{
				u16& idx				= heap->m_ItemCurrPlace.value;
				if (m_iNextHeapIdx != u16_max)
				{
					idx					= m_iNextHeapIdx;
					m_iNextHeapIdx		= u16_max;
				}

				if (m_Heaps.size() <= idx)
					m_Heaps.resize		(idx+1);
				m_Heaps[idx]			= heap;
			}
			else
				m_Heaps.erase			(::std::find(m_Heaps.begin(), m_Heaps.end(), heap));

			if (!smart_cast<CWeapon*>(&O))
				UpdateBulletsVisibility	();

			InvalidateState				();
			break;
		}

		case eWeight:
		{
			if (m_SumWeight == flt_max)
			{
				m_SumWeight				= 0.f;
				for (auto heap : m_Heaps)
					m_SumWeight			+= heap->Weight();
			}
			return						m_SumWeight;
		}
		case eGetAmount:
			return						(float)Amount();
		case eGetBar:
		case eGetFill:
		{
			float fill					= (float)Amount() / (float)Capacity();
			if (type == eGetBar)
				return					(fLess(fill, 1.f)) ? fill : -1.f;
			return						fill;
		}
	}

	return								CModule::aboba(type, data, param);
}

u16 MMagazine::Amount()
{
	if (m_SumAmount == u16_max)
	{
		m_SumAmount						= 0;
		if (m_Heaps.size())
		{
			for (auto I : m_Heaps)
				m_SumAmount				+= I->GetAmmoCount();
		}
	}
	return								m_SumAmount;
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

void MMagazine::loadCartridge(CCartridge CR$ cartridge)
{
	CWeaponAmmo* back_heap				= (Empty()) ? nullptr : m_Heaps.back();
	if (back_heap && back_heap->cNameSect() == cartridge.m_ammoSect && fEqual(back_heap->GetCondition(), cartridge.m_fCondition))
		back_heap->ChangeAmmoCount		(1);
	else
	{
		O.giveItem						(cartridge.m_ammoSect.c_str(), cartridge.m_fCondition);
		m_iNextHeapIdx					= m_Heaps.size();
	}
	InvalidateState						();
}

void MMagazine::LoadCartridge(CWeaponAmmo* ammo)
{
	CCartridge							cartridge;
	ammo->Get							(cartridge);
	loadCartridge						(cartridge);
}

bool MMagazine::GetCartridge(CCartridge& destination, bool expend)
{
	if (Empty())
		return							false;

	InvalidateState						();
	return								m_Heaps.back()->Get(destination, expend);
}
