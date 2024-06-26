#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"
#include "Magazine.h"
#include "GrenadeLauncher.h"
#include "InventoryBox.h"

#include "addon_owner.h"
#include "scope.h"
#include "barrel.h"

class ENGINE_API CMotionDef;

class CWeaponHud;
class CScope;
class CSilencer;
class CGrenadeLauncher;
struct SWpnBriefInfo;

class CWeaponMagazined : public CWeapon
{
private:
	typedef CWeapon inherited;

protected:
	//���� �������� ��������
	shared_str		m_sSndShotCurrent;

	ESoundTypes		m_eSoundShow;
	ESoundTypes		m_eSoundHide;
	ESoundTypes		m_eSoundEmptyClick;
	ESoundTypes		m_eSoundFiremode;
	ESoundTypes		m_eSoundShot;
	ESoundTypes		m_eSoundReload;
	bool			m_sounds_enabled;
	u32				dwUpdateSounds_Frame;

protected:
	virtual void	OnMagazineEmpty();

	virtual void	switch2_Idle();
	virtual void	switch2_Fire();
	virtual void	switch2_Reload();
	virtual void	switch2_Hiding();
	virtual void	switch2_Hidden();
	virtual void	switch2_Showing();

	virtual void	OnShot();

	virtual void	OnEmptyClick();

	virtual void	OnAnimationEnd(u32 state);
	virtual void	OnStateSwitch(u32 S, u32 oldState);

	virtual void	UpdateSounds();
	
protected:
	virtual bool	reloadCartridge		();
	virtual void	ReloadMagazine		();

	virtual void	state_Fire(float dt);
	virtual void	state_Misfire(float dt);

public:
	CWeaponMagazined(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazined();

	virtual void	Load(LPCSTR section);

	void	updataeSilencerKoeffs();
	virtual CWeaponMagazined*cast_weapon_magazined()
	{
		return this;
	}

	virtual void	SetDefaults();
	virtual void	FireStart();
	virtual void	Reload();
			void	StartReload(EWeaponSubStates substate);

	virtual	void	UpdateCL();
	virtual void	net_Destroy();

	virtual bool	Action			(u16 cmd, u32 flags);

	void			GetBriefInfo(SWpnBriefInfo& info);

public:
	virtual void	SetQueueSize(int size);
	virtual bool	StopedAfterQueueFired()
	{
		return m_bStopedAfterQueueFired;
	}
	virtual void	StopedAfterQueueFired(bool value)
	{
		m_bStopedAfterQueueFired = value;
	}

protected:
	//������������ ������ �������, ������� ����� ����������
	int				m_iQueueSize;
	//���������� ������� ����������� ��������
	int				m_iShotNum;
	//����� ������ �������, ��� ����������� ��������, ���������� ������ (������� ��-�� �������)
	int				m_iBaseDispersionedBulletsCount = 0;
	float			m_base_dispersion_shot_time = 0.f;
	//���� ����, ��� �� ������������ ����� ���� ��� ����������
	//����� ������� ��������, ������� ���� ������ � m_iQueueSize
	bool			m_bStopedAfterQueueFired;
	//���� ����, ��� ���� �� ���� ������� �� ������ �������
	//(���� ���� ����� ������ ������ �� ����� � ��������� FireEnd)
	bool			m_bFireSingleShot;
	//������ ��������
	bool			m_bHasDifferentFireModes;
	xr_vector<s8>	m_aFireModes;
	int				m_iCurFireMode;

public:
	void	OnNextFireMode();
	void	OnPrevFireMode();
	bool	HasFireModes()
	{
		return m_bHasDifferentFireModes;
	};
	virtual	int		GetCurrentFireMode()
	{
		//AVO: fixed crash due to original GSC assumption that CWeaponMagazined will always have firemodes specified in configs.
		//return m_aFireModes[m_iCurFireMode];
		if (HasFireModes())
			return m_aFireModes[m_iCurFireMode];
		else
			return 1;
	};

protected:
	virtual bool	install_upgrade_impl	(LPCSTR section, bool test);

protected:
	virtual bool	AllowFireWhileWorking()
	{
		return false;
	}

	//����������� ������� ��� ������������ �������� HUD
			void	PlayAnimIdle() override;
	virtual void	PlayAnimShow();
	virtual void	PlayAnimHide();
	virtual void	PlayAnimReload();
	virtual void	PlayAnimShoot();

	virtual	int		ShotsFired()
	{
		return m_iShotNum;
	}
	virtual float	GetWeaponDeterioration();

	//Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
	HUD_SOUND_COLLECTION_LAYERED m_layered_sounds;
#endif
	//-Alundaio

//xMod altered
public:
	bool								IsRotatingToZoom					C$	();
	float								GetMagazineWeight					C$	();

	void								modify_holder_params				CO$	(float& range, float& fov);

	void								UpdateHudAdditional					O$	(Dmatrix& trans);
	bool								need_renderable						O$	();
	bool								render_item_ui_query				O$	();
	void								render_item_ui						O$	();
	void								ZoomInc								O$	();
	void								ZoomDec								O$	();

//xMod added
private:
	static float						s_ads_shift_step;
	static float						s_ads_shift_max;
	static float						s_recoil_hud_stopping_power_per_shift;
	static float						s_recoil_hud_relax_impulse_per_shift;
	static float						s_recoil_cam_angle_per_delta;
	static float						s_recoil_cam_stopping_power_per_impulse;
	static float						s_recoil_cam_relax_impulse_ratio;

	CScope*								m_selected_scopes[2]					= { NULL, NULL };
	xr_vector<CScope*>					m_attached_scopes						= {};
	u8									m_iron_sights_blockers					= 0;
	u8									m_iron_sights							= 0;
	CMuzzle*							m_muzzle								= nullptr;
	CSilencer*							m_silencer								= nullptr;
	CMagazine*							m_magazine								= nullptr;
	CAddonSlot*							m_magazine_slot							= nullptr;
	CBarrel*							m_barrel								= nullptr;
	bool								m_shot_shell							= false;
	float								m_ads_shift								= 0.f;
	shared_str							m_handguard								= 0;
	bool								m_grip									= false;
	bool								m_usable								= false;

	SRangeNum<u16>						m_IronSightsZeroing;
	u32									m_animation_slot_reloading;
	bool								m_lock_state_reload;
	bool								m_mag_attach_bolt_release;
	shared_str							m_iron_sight_section;

	bool								get_cartridge_from_mag					(CCartridge& dest, bool expand = true);
	void								load_chamber							(bool from_mag);
	void								UpdateSndShot							();
	void								load_muzzle_params						(CMuzzleBase* src);
	void								cycle_scope								(int idx, bool up = true);
	void								on_firemode_switch						();
	void								process_addon							(CAddon* addon, bool attach);
	void								process_barrel							(CBarrel* barrel, bool attach);
	void								process_muzzle							(CMuzzle* muzzle, bool attach);
	void								process_silencer						(CSilencer* muzzle, bool attach);
	void								process_scope							(CScope* scope, bool attach);
	
	bool								iron_sights_up						C$	()		{ return m_iron_sights && m_iron_sights_blockers <= m_iron_sights; }
	
	bool								is_auto_bolt_allowed				C$	();
	bool								hasAmmoToShoot						C$	();
	bool								is_detaching						C$	();

	LPCSTR								anmType		 						CO$	();
	u32									animation_slot						CO$	();

	CCartridge							getCartridgeToShoot					O$	();
	void								OnHiddenItem						O$	();

protected:
	CWeaponHud*							m_hud									= nullptr;
	CWeaponAmmo*						m_current_ammo							= nullptr;

	void								updateRecoil							();
	void								reload_chamber							(CCartridge* dest = nullptr);
	
	bool								has_ammo_for_reload					C$	(int count = 1);
	float								Aboba								O$	(EEventTypes type, void* data, int param);
	Fvector								getFullFireDirection				O$	(CCartridge CR$ c);
	void								SetADS								O$	(int mode);
	
	void							V$	process_addon_modules					(CGameObject& obj, bool attach);

public:
	static float						s_barrel_length_power;
	static void							loadStaticVariables						();

	void								UpdateShadersDataAndSVP					(CCameraManager& camera);
	void								UpdateHudBonesVisibility				(bool status);
	void								loadChamber								(CWeaponAmmo* ammo);
	void								initReload								(CWeaponAmmo* ammo);

	bool								ScopeAttached						C$	()		{ return !m_attached_scopes.empty(); }
	bool								SilencerAttached					C$	()		{ return !!m_silencer; }
	float								getBarrelLength						C$	()		{ return (m_barrel) ? m_barrel->getLength() : 0.f; }
	float								getBarrelLen						C$	()		{ return m_barrel_len; }

	bool								CanTrade							C$	();
	u16									Zeroing								C$	();
	CScope*								getActiveScope						C$	();
	bool								isEmptyChamber 						C$	();
	
	float								CurrentZoomFactor					CO$	(bool for_actor);

	void								OnTaken								O$	();

	bool							V$	Discharge								(CCartridge& destination);
	bool							V$	canTake								C$	(CWeaponAmmo CPC ammo, bool chamber);

	friend class CWeaponHud;
};
