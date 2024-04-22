#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"
#include "Magazine.h"
#include "GrenadeLauncher.h"
#include "InventoryBox.h"

#include "addon_owner.h"
#include "scope.h"

class ENGINE_API CMotionDef;

class CWeaponHud;
class CScope;
class CSilencer;
class CGrenadeLauncher;

//������ ������� ��������� �������������
//����������� ��������, ������, ���� ��������� �������
#define WEAPON_ININITE_QUEUE -1

class CWeaponMagazined : public CWeapon
{
private:
	typedef CWeapon inherited;

protected:
	//���� �������� ��������
	shared_str		m_sSndShotCurrent;

	ESoundTypes		m_eSoundShow;
	ESoundTypes		m_eSoundHide;
	ESoundTypes		m_eSoundCloseSlide;
	ESoundTypes		m_eSoundShot;
	ESoundTypes		m_eSoundEmptyClick;
	ESoundTypes		m_eSoundReload;
#ifdef NEW_SOUNDS //AVO: new sounds go here
	ESoundTypes		m_eSoundReloadEmpty;
	ESoundTypes		m_eSoundReloadMisfire;
#endif //-NEW_SOUNDS
	bool			m_sounds_enabled;
	// General
	//���� ������� ��������� UpdateSounds
	u32				dwUpdateSounds_Frame;

protected:
	virtual void	OnMagazineEmpty();

	virtual void	switch2_Idle();
	virtual void	switch2_Fire();
	virtual void	switch2_Empty();
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
	void			ResetSilencerKoeffs	();

	virtual void	state_Fire(float dt);
	virtual void	state_MagEmpty(float dt);
	virtual void	state_Misfire(float dt);

public:
	CWeaponMagazined(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponMagazined();

	virtual void	Load(LPCSTR section);

	void	LoadSilencerKoeffs(LPCSTR sect);
	virtual CWeaponMagazined*cast_weapon_magazined()
	{
		return this;
	}

	virtual void	SetDefaults();
	virtual void	FireStart();
	virtual void	Reload();
	virtual void	StartReload();

	virtual	void	UpdateCL();
	virtual void	net_Destroy();
	virtual void	net_Export(NET_Packet& P);
	virtual void	net_Import(NET_Packet& P);

	virtual void	OnH_A_Chield();

	virtual bool	Action			(u16 cmd, u32 flags);
	bool			IsAmmoAvailable	();

	virtual bool	GetBriefInfo(II_BriefInfo& info);

public:
	virtual bool	SwitchMode();
	virtual bool	SingleShotMode()
	{
		return 1 == m_iQueueSize;
	}
	virtual void	SetQueueSize(int size);
	IC		int		GetQueueSize() const
	{
		return m_iQueueSize;
	};
	virtual bool	StopedAfterQueueFired()
	{
		return m_bStopedAfterQueueFired;
	}
	virtual void	StopedAfterQueueFired(bool value)
	{
		m_bStopedAfterQueueFired = value;
	}
	virtual float	GetFireDispersion(float cartridge_k, bool for_crosshair = false);

protected:
	//������������ ������ �������, ������� ����� ����������
	int				m_iQueueSize;
	//���������� ������� ����������� ��������
	int				m_iShotNum;
	//����� ������ �������, ��� ����������� ��������, ���������� ������ (������� ��-�� �������)
	int				m_iBaseDispersionedBulletsCount;
	//�������� ������ ��������, �� ������� �� ������ ������ (������� ��-�� �������)
	float			m_fBaseDispersionedBulletsSpeed;
	//�������� ������ ��������� ��������
	float			m_fOldBulletSpeed;
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
	int				m_iPrefferedFireMode;

	//���������� ��������� �������������
	//������ ������ ����� ��������
	bool			m_bLockType;
	CWeaponAmmo*	m_pCurrentAmmo;

public:
	virtual void	OnZoomIn();
	virtual void	OnZoomOut();
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

	virtual void	save(NET_Packet &output_packet);
	virtual void	load(IReader &input_packet);


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

	virtual void	FireBullet(const Fvector& pos,
		const Fvector& dir,
		float fire_disp,
		const CCartridge& cartridge,
		u16 parent_id,
		u16 weapon_id,
		bool send_hit);
	//AVO: for custom added sounds check if sound exists
	bool WeaponSoundExist(LPCSTR section, LPCSTR sound_name);

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

	void								UpdateHudAdditional					O$	(Fmatrix& trans);
	bool								need_renderable						O$	();
	bool								render_item_ui_query				O$	();
	void								render_item_ui						O$	();
	void								ZoomInc								O$	();
	void								ZoomDec								O$	();

//xMod added
private:
	CScope*								m_selected_scopes[2]					= { NULL, NULL };
	xr_vector<CScope*>					m_attached_scopes						= {};
	u8									m_iron_sights_blockers					= 0;

	CSilencer*							m_pSilencer;
	CAddonSlot*							m_pMagazineSlot;
	float								m_ReloadHalfPoint;
	float								m_ReloadEmptyHalfPoint;
	float								m_ReloadPartialPoint;
	SRangeNum<u16>						m_IronSightsZeroing;
	bool								m_lower_iron_sights_on_block;

	bool								get_cartridge_from_mag					(CCartridge& dest, bool expand = true);
	void								reload_chamber							();
	void								UpdateSndShot							();
	void								UpdateBonesVisibility					();
	void								ProcessMagazine							(CMagazine* mag, bool attach);
	void								ProcessSilencer							(CSilencer* sil, bool attach);
	void								process_scope							(CScope* scope, bool attach);
	void								cycle_scope								(int idx, bool up = true);
	void								InitRotateTime							();
	
	bool								hasAmmoToShoot						C$	();
	bool								is_detaching						C$	();
	bool								is_empty 							CO$	();
	CCartridge							getCartridgeToShoot					O$	();
	void								OnHiddenItem						O$	();

protected:
	CWeaponHud*							m_hud;
	CMagazine*							m_pMagazine;
	CWeaponAmmo*						m_pCartridgeToReload;

	void								updateRecoil							();

	float								Aboba								O$	(EEventTypes type, void* data, int param);
	Fvector								getFullFireDirection				O$	(CCartridge CR$ c);

public:
	void								UpdateShadersDataAndSVP					(CCameraManager& camera);
	void								UpdateHudBonesVisibility				();
	void								loadChamber								(CWeaponAmmo* cartridges);
	void								startReload								(CWeaponAmmo* cartridges);

	bool								ScopeAttached						C$	()		{ return !m_attached_scopes.empty(); }
	bool								SilencerAttached					C$	()		{ return !!m_pSilencer; }
	CAddonSlot CP$						MagazineSlot						C$	()		{ return m_pMagazineSlot; }

	bool								CanTrade							C$	();
	u16									Zeroing								C$	();
	CScope*								getActiveScope						C$	();

	float								CurrentZoomFactor					CO$	(bool for_actor);

	void								OnTaken								O$	();
	void								onMotionSignal						O$	();

	void							V$	process_addon							(CAddon* addon, bool attach);
	bool							V$	Discharge								(CCartridge& destination);

	friend class CWeaponHud;
};
