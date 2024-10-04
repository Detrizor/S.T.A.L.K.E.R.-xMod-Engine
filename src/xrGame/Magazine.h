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
	const shared_str					m_ammo_slot_type;

	xr_vector<CWeaponAmmo*>				m_heaps									= {};
	u16									m_amount								= 0;
	float								m_weight								= 0.f;

	void								update_bullets_visibility				();
	void								update_hud_bullets_visibility			();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	void								loadCartridge							(CCartridge CR$ cartridge, int count = 1);
	void								loadCartridge							(CWeaponAmmo* ammo);
	bool								getCartridge							(CCartridge& destination, bool expend = true);
	void								setCondition							(float val);
	
	bool								canTake								C$	(CWeaponAmmo CPC ammo);
	
	u16									Amount								C$	()		{ return m_amount; }
	u16									Capacity							C$	()		{ return m_capacity; };	
	bool								Empty								C$	()		{ return m_amount == 0; }
	bool								Full								C$	()		{ return m_amount == m_capacity; }
	shared_str CR$						attachAnm							C$	()		{ return m_attach_anm; }
	shared_str CR$						detachAnm							C$	()		{ return m_detach_anm; }
};
