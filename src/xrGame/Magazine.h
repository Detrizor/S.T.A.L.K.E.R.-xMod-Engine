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
	void								sOnChild							O$	(CGameObject* obj, bool take);
	void								sUpdateHudBonesVisibility			O$	();
	float								sSumItemData						O$	(EItemDataTypes type);
	xoptional<float>					sGetAmount							O$	()		{ return static_cast<float>(m_amount); }
	xoptional<float>					sGetFill							O$	()		{ return static_cast<float>(m_amount) / static_cast<float>(m_capacity); }
	xoptional<float>					sGetBar								O$	();

private:
	const u16							m_capacity;
	const bool							m_bullets_visible;
	const shared_str					m_attach_anm;
	const shared_str					m_detach_anm;
	const shared_str					m_ammo_slot_type;

	xr_vector<CWeaponAmmo*>				m_heaps									= {};
	u16									m_amount								= 0;
	float								m_weight								= 0.f;

	bool								m_vis;

	void								update_bullets_visibility				();
	void								update_hud_bullets_visibility			();
	void								register_heap							(CWeaponAmmo* heap, bool insert);

	CWeaponAmmo*						get_ammo							C$	();

public:
	void								loadCartridge							(CCartridge CR$ cartridge, int count = 1);
	void								loadCartridge							(CWeaponAmmo* ammo);
	xoptional<CCartridge>				getCartridge							(bool expend = true);
	void								setCondition							(float val, bool recursive);
	
	bool								canTake								C$	(CWeaponAmmo CPC ammo);
	CWeaponAmmo const*					getAmmo								C$	();
	
	u16									Amount								C$	()		{ return m_amount; }
	u16									Capacity							C$	()		{ return m_capacity; };	
	bool								Empty								C$	()		{ return m_amount == 0; }
	bool								Full								C$	()		{ return m_amount == m_capacity; }
	shared_str CR$						attachAnm							C$	()		{ return m_attach_anm; }
	shared_str CR$						detachAnm							C$	()		{ return m_detach_anm; }
};
