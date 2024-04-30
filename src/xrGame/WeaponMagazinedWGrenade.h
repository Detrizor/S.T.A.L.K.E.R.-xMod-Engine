#pragma once
#include "weaponmagazined.h"
#include "rocketlauncher.h"


class CWeaponFakeGrenade;


class CWeaponMagazinedWGrenade : public CWeaponMagazined,
								 public CRocketLauncher
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponMagazinedWGrenade	(ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazinedWGrenade	();

	virtual void	Load				(LPCSTR section);
	
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	virtual void	net_Destroy			();
	virtual void	net_Export			(NET_Packet& P);
	virtual void	net_Import			(NET_Packet& P);
	
	virtual void	OnH_B_Independent	(bool just_before_destroy);

	virtual void	save				(NET_Packet &output_packet);
	virtual void	load				(IReader &input_packet);

			void	LaunchGrenade			();
	
	virtual void	OnStateSwitch	(u32 S, u32 oldState);
	
	virtual void	state_Fire		(float dt);
	virtual void	OnShot			();
	virtual void	OnEvent			(NET_Packet& P, u16 type);
	virtual void	ReloadMagazine	();

	virtual bool	Action			(u16 cmd, u32 flags);

	virtual void	UpdateSounds	();

	//переключение в режим подствольника
	virtual bool	SwitchMode		();
	void			PerformSwitchGL	();
	void			OnAnimationEnd	(u32 state);
	virtual void	OnMagazineEmpty	();

	virtual bool	IsNecessaryItem	    (const shared_str& item_sect);

	bool			PlayAnimModeSwitch	();
	
private:
	virtual	void	net_Spawn_install_upgrades	( Upgrades_type saved_upgrades );
	virtual bool	install_upgrade_impl		( LPCSTR section, bool test );

private:
	xr_vector<shared_str>				m_grenade_types							= {};
	u8									m_grenade_type							= 0;
	bool								m_bGrenadeMode							= false;

	CGrenadeLauncher CP$				m_pLauncher								= NULL;
	Fvector								m_muzzle_position_gl					= vZero;
	shared_str							m_flame_particles_gl_name				= 0;
	CParticlesObject*					m_flame_particles_gl					= NULL;
	std::unique_ptr<CCartridge>			m_grenade								= nullptr;

	void								ProcessGL								(CGrenadeLauncher* gl, bool attach);

	void								shoot_grenade							();
	void								start_flame_particles_gl				();
	void								stop_flame_particles_gl					();
	void								update_flame_particles_gl				();
	
	int									GetAmmoElapsed						CO$	()		{ return (int)!!m_grenade; }

	bool								AltHandsAttach						CO$	();
	bool								HasAltAim							CO$	();

	void								SetADS								O$	(int mode);
	float								Aboba								O$	(EEventTypes type, void* data, int param);
	void								UpdateCL							O$	();
	void								process_addon						O$	(CAddon* addon, bool attach);
	void								PlayAnimReload						O$	();
	bool								Discharge							O$	(CCartridge& destination);

public:
	void								SetGrenade								(u8 cnt);

	bool								isGrenadeMode						C$	()		{ return m_bGrenadeMode; }
	u8									GetGrenade							C$	();
	
	bool								canTake								CO$	(CWeaponAmmo CPC ammo, bool chamber);
};
