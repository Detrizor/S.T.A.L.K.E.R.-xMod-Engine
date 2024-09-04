#pragma once

#include "module.h"

class CInventoryStorage;
class CWeaponAmmo;
class CCartridge;
class CGameObject;

class MMagazine : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mMagazine; }

public:
										MMagazine								(CGameObject* obj, shared_str CR$ section);

private:
	const u16							m_capacity;
	const bool							m_bullets_visible;
	const shared_str					m_attach_anm;
	const shared_str					m_detach_anm;

	xr_vector<CWeaponAmmo*>				m_Heaps									= {};
	u16									m_iNextHeapIdx							= u16_max;
	xr_vector<shared_str>				m_ammo_types							= {};
	u16									m_SumAmount;
	float								m_SumWeight;

	void								InvalidateState							();
	void								UpdateBulletsVisibility					();
	void								UpdateHudBulletsVisibility				();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	u16									Amount									();
	bool								CanTake									(CWeaponAmmo CPC ammo);
	void								loadCartridge							(CCartridge CR$ cartridge, int count = 1);
	void								LoadCartridge							(CWeaponAmmo* ammo);
	bool								GetCartridge							(CCartridge& destination, bool expend = true);
	void								setCondition							(float val);

	bool								Full									()		{ return (Amount() == Capacity()); }
	u16									Capacity							C$	()		{ return m_capacity; };	
	bool								Empty								C$	()		{ return m_Heaps.empty(); }
	shared_str CR$						attachAnm							C$	()		{ return m_attach_anm; }
	shared_str CR$						detachAnm							C$	()		{ return m_detach_anm; }
};
