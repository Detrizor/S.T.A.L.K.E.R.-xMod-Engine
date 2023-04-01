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
	u16									m_iHeapsCount;
	xr_vector<shared_str>				m_ammo_types;
	u16									m_capacity;

	void								UpdateBulletsVisibility					();

	float								_GetAmount							CO$	()		{ return (float)Amount(); }
	float								_GetFill							CO$	()		{ return (float)Amount() / (float)Capacity(); }
	float								_GetBar								CO$	()		{ return (Amount() < Capacity()) ? _GetFill() : -1.f; }

	float								_Weight								CO$	();
	float								_Cost								CO$	();

	void								_OnChild							O$	(CObject* obj, bool take);
	void								_UpdateHudBonesVisibility			O$	();

public:
	u16									Capacity							C$	()		{ return m_capacity; };	
	bool								Full								C$	()		{ return (Amount() == Capacity()); }
	bool								Empty								C$	()		{ return m_Heaps.empty(); }

	void								LoadCartridge							(CWeaponAmmo* ammo);
	bool								GetCartridge							(CCartridge& destination, bool expend = true);

	u16									Amount								C$	();
	bool								CanTake								C$	(CWeaponAmmo CPC ammo);
};
