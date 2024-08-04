#pragma once

#include "module.h"

class CWeaponMagazinedWGrenade;
class CAddonSlot;

class MGrenadeLauncher : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mLauncher; }

public:
										MGrenadeLauncher						(CGameObject* obj, shared_str CR$ section);

private:
	float								m_fGrenadeVel;
	shared_str							m_sFlameParticles;
	Dvector								m_sight_offset[2];
	
	CAddonSlot*							m_slot									= nullptr;
	CWeaponMagazinedWGrenade*			m_wpn									= nullptr;
	
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	void								setWeapon								(CWeaponMagazinedWGrenade* wpn)		{ m_wpn = wpn; }

	float								GetGrenadeVel						C$	()		{ return m_fGrenadeVel; }
	shared_str CR$						FlameParticles						C$	()		{ return m_sFlameParticles; }
	Dvector CP$							getSightOffset						C$	()		{ return m_sight_offset; }

	friend class CWeaponMagazinedWGrenade;
};
