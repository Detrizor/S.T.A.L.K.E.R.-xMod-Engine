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

	virtual void	FireEnd					();
			void	LaunchGrenade			();
	
	virtual void	OnStateSwitch	(u32 S, u32 oldState);
	
	virtual void	switch2_Reload	();
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
	virtual bool	GetBriefInfo			(II_BriefInfo& info);

	virtual bool	IsNecessaryItem	    (const shared_str& item_sect);

	//виртуальные функции для проигрывания анимации HUD
	virtual void	PlayAnimShow		();
	virtual void	PlayAnimHide		();
	virtual void	PlayAnimReload		();
	virtual void	PlayAnimIdle		();
	virtual void	PlayAnimShoot		();
	virtual bool	PlayAnimModeSwitch	();
	virtual void	PlayAnimBore		();
	
private:
	virtual	void	net_Spawn_install_upgrades	( Upgrades_type saved_upgrades );
	virtual bool	install_upgrade_impl		( LPCSTR section, bool test );
	virtual	bool	install_upgrade_ammo_class	( LPCSTR section, bool test );
	
			int		GetAmmoCount2				( u8 ammo2_type ) const;

public:
	//дополнительные параметры патронов 
	//для подствольника
//-	CWeaponAmmo*			m_pAmmo2;
	xr_vector<shared_str>	m_ammoTypes2;
	u8						m_ammoType2;

	int						iMagazineSize2;
	xr_vector<CCartridge>	m_magazine2;

	bool					m_bGrenadeMode;

	CCartridge				m_DefaultCartridge2;

	virtual void UpdateGrenadeVisibility(bool visibility);

	virtual	xr_vector<CCartridge>&	Magazine();
			u8						GetGrenade();
			void					SetGrenade(u8 cnt);

private:
	CGrenadeLauncher CP$						m_pLauncher;

	void								ProcessGL								(CGrenadeLauncher* gl, bool attach);

protected:
	bool								AltHandsAttachRotation				CO$	();

	BOOL								Chamber								CO$	();
	bool								HasAltAim							CO$	();

	void								SetADS								O$	(int mode);
	void								OnMotionHalf						O$	();
	float								Aboba								O$	(EEventTypes type, void* data, int param);
};
