#pragma once

#include "module.h"

class CInventoryStorage;
class CWeaponAmmo;
class CCartridge;
class CGameObject;

class CMagazine : public CModule
{
public:
										CMagazine								(CGameObject* obj);

private:
	xr_vector<CWeaponAmmo*>				m_Heaps;
	u16									m_iNextHeapIdx;
	xr_vector<shared_str>				m_ammo_types;
	u16									m_capacity;
	u16									m_SumAmount;
	float								m_SumWeight;

	void								InvalidateState							();
	void								UpdateBulletsVisibility					();
	void								UpdateHudBulletsVisibility				();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	u16									Amount									();
	bool								CanTake									(CWeaponAmmo CPC ammo);
	void								LoadCartridge							(CWeaponAmmo* ammo);
	bool								GetCartridge							(CCartridge& destination, bool expend = true);

	bool								Full									()		{ return (Amount() == Capacity()); }
	u16									Capacity							C$	()		{ return m_capacity; };	
	bool								Empty								C$	()		{ return m_Heaps.empty(); }
};
