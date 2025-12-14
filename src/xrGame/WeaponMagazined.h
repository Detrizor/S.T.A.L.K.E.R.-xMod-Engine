#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"
#include "Magazine.h"
#include "GrenadeLauncher.h"
#include "InventoryBox.h"

#include "addon_owner.h"
#include "scope.h"
#include "weapon_hud.h"
#include "weapon_chamber.h"

class ENGINE_API CMotionDef;

class CWeaponHud;
class CSilencer;
class MGrenadeLauncher;
class MFoldable;
class MMuzzle;
struct SWpnBriefInfo;

class CWeaponMagazined : public CWeapon
{
private:
	typedef CWeapon inherited;

protected:
	//звук текущего выстрела
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
	virtual void	ReloadMagazine		();

	virtual void	state_Fire(float dt);
	virtual void	state_Misfire(float dt);

public:
	CWeaponMagazined(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN);

	virtual void	Load(LPCSTR section);

	virtual CWeaponMagazined*cast_weapon_magazined()
	{
		return this;
	}

	virtual void	SetDefaults();
	virtual void	FireStart();
	virtual void	Reload();
	virtual void	StartReload(EWeaponSubStates substate);

	virtual	void	UpdateCL();

	virtual bool	Action			(u16 cmd, u32 flags);

	void			GetBriefInfo(SWpnBriefInfo& info);

public:
	virtual void	SetQueueSize(int size = 0);
	virtual bool	StopedAfterQueueFired()
	{
		return m_bStopedAfterQueueFired;
	}
	virtual void	StopedAfterQueueFired(bool value)
	{
		m_bStopedAfterQueueFired = value;
	}

protected:
	//максимальный размер очереди, которой можно стрельнуть
	int				m_iQueueSize						= 1;
	//количество реально выстреляных патронов
	int				m_iShotNum							= 0;
	//после какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-за Абакана)
	int				m_iBaseDispersionedBulletsCount		= 0;
	float			m_base_dispersion_shot_time			= 0.f;
	//флаг того, что мы остановились после того как выстреляли
	//ровно столько патронов, сколько было задано в m_iQueueSize
	bool			m_bStopedAfterQueueFired;
	//флаг того, что хотя бы один выстрел мы должны сделать
	//(даже если очень быстро нажали на курок и вызвалось FireEnd)
	bool			m_bFireSingleShot					= false;
	//режимы стрельбы
	bool			m_bHasDifferentFireModes			= false;
	xr_vector<int>	m_aFireModes						= {};
	int				m_iCurFireMode						= 0;
	
	void			switch_firemode						(int val);
	void			set_firemode						(int val);

public:
	virtual	int		GetCurrentFireMode()
	{
		return (m_aFireModes.size()) ? m_aFireModes[m_iCurFireMode] : 1;
	};

	void			OnH_B_Chield						() override;

protected:
	virtual bool	install_upgrade_impl	(LPCSTR section, bool test);

protected:
	virtual bool	AllowFireWhileWorking()
	{
		return false;
	}

	//виртуальные функции для проигрывания анимации HUD
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

	void								modify_holder_params				CO$	(float& range, float& fov);

	void								UpdateHudAdditional					O$	(Dmatrix& trans);
	bool								need_renderable						O$	();
	bool								render_item_ui_query				O$	();
	void								render_item_ui						O$	();
	void								ZoomInc								O$	();
	void								ZoomDec								O$	();

//xMod added
protected:
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);
	void								sOnAddon							O$	(MAddon* addon, int attach_type);
	void								sUpdateSlotsTransform				O$	();

private:
	static float						s_ads_shift_step;
	static float						s_ads_shift_max;
	static float						s_recoil_hud_stopping_power_per_shift;
	static float						s_recoil_hud_relax_impulse_per_shift;
	static float						s_recoil_cam_angle_per_delta;
	static float						s_recoil_cam_stopping_power_per_impulse;
	static float						s_recoil_cam_relax_impulse_ratio;
	static float						s_recoil_processing_time_delta_step;

	static float						m_stock_accuracy_modifier_absent;

	MScope*								m_selected_scopes[2]					= { NULL, NULL };
	xr_vector<MScope*>					m_attached_scopes						= {};
	float								m_ads_shift								= 0.f;
	Dvector								m_align_front							= dZero;
	float								m_barrel_length							= 0.f;
	bool								m_cocked								= false;

	u32									m_animation_slot_reloading;
	bool								m_lock_state_reload;
	bool								m_lock_state_shooting;
	bool								m_mag_attach_bolt_release;
	bool								m_bolt_catch;
	SScriptAnm							m_empty_click_anm;
	SScriptAnm							m_bolt_pull_anm;
	SScriptAnm							m_firemode_anm;
	
	void								load_firemodes							(LPCSTR str);
	void								UpdateSndShot							();
	void								cycle_scope								(int idx, bool up = true);
	void								on_firemode_switch						();
	void								on_reticle_switch						();
	void								process_addon							(MAddon* addon, bool attach);
	void								process_muzzle							(MMuzzle* muzzle, bool attach);
	void								process_silencer						(CSilencer* muzzle, bool attach);
	void								process_magazine						(MMagazine* magazine, bool attach);
	void								process_scope							(MScope* scope, bool attach);
	void								process_align_front						(CGameObject* obj, bool attach);

	bool								is_auto_bolt_allowed				C$	();
	bool								need_loaded_anm						C$	();
	xoptional<CCartridge>				get_cartridge_from_mag				C$	();

	LPCSTR								anmType		 						CO$	();
	u32									animation_slot						CO$	();

	void								prepare_cartridge_to_shoot			O$	();
	
	virtual bool						has_ammo_to_shoot					C$	();
	virtual bool						has_mag_with_ammo					C$	();

protected:
	CWeaponChamber						m_chamber;

	CWeaponAmmo*						m_current_ammo							= nullptr;
	bool								m_locked								= false;
	MMagazine*							m_magazine								= nullptr;
	CAddonSlot*							m_magazine_slot							= nullptr;
	bool								m_grip									= false;

	void								updateRecoil							(float dt, float accuracy);
	void								process_addon_data						(CGameObject& obj, shared_str CR$ section, bool attach);
	int									try_consume_ammo						(int count);
	bool								on_bolt_lock							();
	bool								has_ammo_for_reload						(int count = 1);

	Fvector								getFullFireDirection				O$	(CCartridge CR$ c);
	void								setADS								O$	(int mode);
	void								OnHiddenItem						O$	();
	
	void							V$	process_addon_modules					(CGameObject& obj, bool attach);
	void							V$	process_foregrip						(CGameObject& obj, LPCSTR type, bool attach);

public:
	static float						s_barrel_length_power;
	static void							loadStaticData							();

	MMagazine*							getMagazine								()		{ return m_magazine; }
	
	void								unloadChamber							(MAddon* chamber = nullptr);
	void								loadChamber								(CWeaponAmmo* ammo = nullptr);
	void								onFold									(MFoldable CP$ foldable, bool new_status);

	bool								ScopeAttached						C$	()		{ return !m_attached_scopes.empty(); }
	bool								SilencerAttached					C$	()		{ return !!m_silencer; }
	float								getBarrelLength						C$	()		{ return m_barrel_length; }
	float								getBarrelLen						C$	()		{ return m_barrel_len; }
	float								charged								C$	()		{ return m_chamber && m_chamber.loaded(); }
	float								uncharged							C$	()		{ return m_chamber && !m_chamber.loaded(); }

	bool								CanTrade							C$	();
	u16									Zeroing								C$	();
	MScope*								getActiveScope						C$	();
	void								updateSVP							C$	();
	
	float								getZoom								CO$	();
	int									GetAmmoElapsed						CO$	();
	int									GetAmmoMagSize						CO$	();

	void								OnTaken								O$	();

	virtual	bool						tryTransfer								(MAddon* addon, bool attach);

protected:
	xptr<CWeaponHud> m_pHud{ nullptr };

	friend class CWeaponHud;
	friend class CWeaponChamber;
};
