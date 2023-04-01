#include "stdafx.h"
#include "Magazine.h"
#include "Level.h"
#include "player_hud.h"
#include "../Include/xrRender/Kinematics.h"
#include "WeaponAmmo.h"
#include "Weapon.h"

CMagazine::CMagazine(CGameObject* obj) : CModule(obj)
{
	m_Heaps.clear						();
	m_iNextHeapIdx						= NO_ID;
	m_iHeapsCount						= 0;

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
}

shared_str bullets						= "bullets";
extern void UpdateBoneVisibility		(IKinematics* pVisual, const shared_str& bone_name, bool status);
void CMagazine::UpdateBulletsVisibility()
{
	bool vis							= !Empty();
	cast<PIItem>()->SetInvIconType		((u8)vis);

	IKinematics* pVisual				= smart_cast<IKinematics*>(O.Visual());
	pVisual->CalculateBones_Invalidate	();
	UpdateBoneVisibility				(pVisual, bullets, vis);
	pVisual->CalculateBones_Invalidate	();
	pVisual->CalculateBones				(TRUE);
}

float CMagazine::_Weight() const
{
	float res							= 0.f;
	for (auto heap : m_Heaps)
		res								+= heap->Weight();
	return								res;
}

float CMagazine::_Cost() const
{
	float res							= 0.f;
	for (auto heap : m_Heaps)
		res								+= heap->Cost();
	return								res;
}

void CMagazine::_OnChild o$(CObject* obj, bool take)
{
	CWeaponAmmo* heap					= smart_cast<CWeaponAmmo*>(obj);
	if (!heap)
		return;
	
	u16& idx							= heap->m_ItemCurrPlace.value;
	if (m_iNextHeapIdx != NO_ID)
	{
		idx								= m_iNextHeapIdx;
		m_iNextHeapIdx					= NO_ID;
	}
	
	if (take)
	{
		if (m_Heaps.size() <= idx)
			m_Heaps.resize				(idx+1);
		m_Heaps[idx]					= heap;
		m_iHeapsCount++;
	}
	else
	{
		m_Heaps[idx]					= NULL;
		m_iHeapsCount--;
	}

	if (!smart_cast<CWeapon*>(&O))
		UpdateBulletsVisibility			();
}

void CMagazine::_UpdateHudBonesVisibility()
{
	cast<CHudItem*>()->HudItemData()->set_bone_visible(bullets, (BOOL)!Empty(), TRUE);
}

void CMagazine::LoadCartridge(CWeaponAmmo* ammo)
{
	CWeaponAmmo* back_heap				= (Empty()) ? NULL : m_Heaps.back();
	if (back_heap && back_heap->cNameSect() == ammo->cNameSect() && fEqual(back_heap->GetCondition(), ammo->GetCondition()))
		back_heap->ChangeAmmoCount		(1);
	else
	{
		O.GiveAmmo						(*ammo->cNameSect(), 1, ammo->GetCondition());
		m_iNextHeapIdx					= m_iHeapsCount;
	}
	ammo->ChangeAmmoCount				(-1);
}

bool CMagazine::GetCartridge(CCartridge& destination, bool expend)
{
	return								(Empty()) ? false : m_Heaps.back()->Get(destination, expend);
}

u16 CMagazine::Amount() const
{
	u16 res								= 0;
	if (m_Heaps.size())
	{
		for (auto I : m_Heaps)
			res							+= I->GetAmmoCount();
	}
	return								res;
}

bool CMagazine::CanTake(CWeaponAmmo CPC ammo) const
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
