#pragma once
#include "weaponmagazined.h"
#include "rocketlauncher.h"

class CWeaponFakeGrenade;

class CWeaponMagazinedWGrenade :
	public CWeaponMagazined,
	public CRocketLauncher
{
	typedef CWeaponMagazined inherited;

public:
										CWeaponMagazinedWGrenade				(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN) {}

protected:
	void								Load								O$	(LPCSTR section);

	void								net_Destroy							O$	();
	void								UpdateCL							O$	();
	void								OnEvent								O$	(NET_Packet& P, u16 type);
	bool								Action								O$	(u16 cmd, u32 flags);
	
	void								OnStateSwitch						O$	(u32 S, u32 oldState);
	void								UpdateSounds						O$	();
	void								OnAnimationEnd						O$	(u32 state);
	bool								IsNecessaryItem						O$	(const shared_str& item_sect);
	void								PlayAnimReload						O$	();

	bool								HasAltAim							CO$	();
	int									ADS									CO$	();

	float								Aboba								O$	(EEventTypes type, void* data, int param);
	void								process_addon_modules				O$	(CGameObject& obj, bool attach);
	void								process_foregrip					O$	(CGameObject& obj, LPCSTR type, bool attach);

private:
	bool								m_bGrenadeMode							= false;
	MGrenadeLauncher CP$				m_pLauncher								= nullptr;
	shared_str							m_flame_particles_gl_name				= 0;
	CParticlesObject*					m_flame_particles_gl					= nullptr;
	CWeaponAmmo*						m_grenade								= nullptr;
	Fvector								m_muzzle_point_gl						= vZero;
	Fvector								m_fire_point_gl							= vZero;
	u32									m_fire_point_gl_update_frame			= 0;


	void								start_flame_particles_gl				();
	void								stop_flame_particles_gl					();
	void								update_flame_particles_gl				();
	bool								switch_mode								();
	void								launch_grenade							();
	void								process_gl								(MGrenadeLauncher* gl, bool attach);
	Fvector CR$							fire_point_gl							();
	
	bool								tryTransfer							O$	(MAddon* addon, bool attach);

protected:
	LPCSTR								get_anm_prefix						CO$	();

public:
	bool								isGrenadeMode						C$	()		{ return m_bGrenadeMode; }
};
