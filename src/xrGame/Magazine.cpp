#include "stdafx.h"
#include "Magazine.h"
#include "Level.h"
#include "player_hud.h"
#include "../Include/xrRender/Kinematics.h"

CMagazine::CMagazine()
{
	m_Heaps.clear					();
	m_iNextHeapIdx					= NO_ID;
	m_iHeapsCount					= 0;
    m_ammo_types.clear				();
	m_capacity						= 0;
}

DLL_Pure* CMagazine::_construct()
{
	m_object						= smart_cast<CGameObject*>(this);
	m_storage						= smart_cast<CInventoryStorage*>(this);
	return							m_object;
}

void CMagazine::Load(LPCSTR section)
{
	m_capacity						= pSettings->r_u32(section, "capacity");
    LPCSTR S						= pSettings->r_string(section, "ammo_class");
    string128						_ammoItem;
    int count						= _GetItemCount(S);
    for (int it = 0; it < count; ++it)
    {
        _GetItem					(S, it, _ammoItem);
		m_ammo_types.push_back		(_ammoItem);
    }
}

void CMagazine::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	CWeaponAmmo* heap				= smart_cast<CWeaponAmmo*>(itm);
	if (!heap)
		return;
	
	u16& idx						= heap->m_ItemCurrPlace.value;
	if (m_iNextHeapIdx != NO_ID)
	{
		idx							= m_iNextHeapIdx;
		m_iNextHeapIdx				= NO_ID;
	}
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		if (m_Heaps.size() <= idx)
			m_Heaps.resize			(idx+1);
		m_Heaps[idx]				= heap;
		m_iHeapsCount++;
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		m_Heaps[idx]				= NULL;
		m_iHeapsCount--;
		break;
	}
}

u16 CMagazine::Amount() const
{
	u16 res							= 0;
	if (m_Heaps.size())
	for (auto I : m_Heaps)
		res							+= I->GetAmmoCount();
	return							res;
}

bool CMagazine::Empty() const
{
	return							!m_Heaps.size();
}

bool CMagazine::CanTake(CWeaponAmmo* ammo) const
{
	if (Amount() == Capacity())
		return						false;
	for (auto& I : m_ammo_types)
	{
		if (I == ammo->m_section_id)
			return					true;
	}
	return							false;
}

void CMagazine::LoadCartridge(CWeaponAmmo* ammo)
{
	CWeaponAmmo* back_heap			= (m_Heaps.size()) ? m_Heaps.back() : NULL;
	if (back_heap && back_heap->cNameSect() == ammo->cNameSect() && fEqual(back_heap->GetCondition(), ammo->GetCondition()))
		back_heap->ChangeAmmoCount	(1);
	else
	{
		m_object->GiveAmmo			(*ammo->cNameSect(), 1, ammo->GetCondition());
		m_iNextHeapIdx				= m_iHeapsCount;
	}
	ammo->ChangeAmmoCount			(-1);
}

bool CMagazine::GetCartridge(CCartridge& destination, bool expend)
{
	if (!m_Heaps.size())
		return						false;
	CWeaponAmmo* back_heap			= m_Heaps.back();
	return							back_heap->Get(destination, expend);
}

float CMagazine::Weight() const
{
	float res						= 0.f;
	for (auto heap : m_Heaps)
		res							+= heap->Weight();
	return							res;
}

u32 CMagazine::Cost() const
{
	u32 res							= 0.f;
	for (auto heap : m_Heaps)
		res							+= heap->Cost();
	return							res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DLL_Pure* CMagazineObject::_construct()
{
	inherited::_construct			();
	CMagazine::_construct			();
	return							this;
}

void CMagazineObject::Load(LPCSTR section)
{
	inherited::Load					(section);
	CMagazine::Load					(section);
}

void CMagazineObject::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent				(P, type);
	
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		UpdateBulletsVisibility		();
	}
}

shared_str bullets					= "bullets";
extern void UpdateBoneVisibility	(IKinematics* pVisual, const shared_str& bone_name, bool status);
void CMagazineObject::UpdateBulletsVisibility()
{
	bool vis						= !Empty();
	SetInvIconType					((u8)vis);

	IKinematics* pVisual			= smart_cast<IKinematics*>(Visual());
	pVisual->CalculateBones_Invalidate();
	UpdateBoneVisibility			(pVisual, bullets, vis);
	pVisual->CalculateBones_Invalidate();
	pVisual->CalculateBones			(TRUE);
}

void CMagazineObject::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	inherited::OnEventImpl			(type, id, itm, dont_create_shell);
	CMagazine::OnEventImpl			(type, id, itm, dont_create_shell);
}

void CMagazineObject::UpdateHudBonesVisibility()
{
	HudItemData()->set_bone_visible(bullets, (BOOL)!Empty(), TRUE);
}

float CMagazineObject::Weight() const
{
	float res						= inherited::Weight();
	res								+= CMagazine::Weight();
	return							res;
}

u32 CMagazineObject::Cost() const
{
	u32 res							= inherited::Cost();
	res								+= CMagazine::Cost();
	return							res;
}