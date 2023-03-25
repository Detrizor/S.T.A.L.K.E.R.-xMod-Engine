#pragma once

#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"
#include "Magazine.h"
#include "GrenadeLauncher.h"
#include "InventoryBox.h"

#include "xmod\scope.h"
#include "xmod\silencer.h"
#include "xmod\addon_owner.h"

class ENGINE_API CMotionDef;
class CScope;
class CSilencer;
class CGrenadeLauncher;

//размер очереди считается бесконечность
//заканчиваем стрельбу, только, если кончились патроны
#define WEAPON_ININITE_QUEUE -1

class CWeaponMagazined : public CWeapon,
	public CAddonOwner
{
private:
    typedef CWeapon inherited;

protected:
	//звук текущего выстрела
	shared_str		m_sSndShotCurrent;

    ESoundTypes		m_eSoundShow;
    ESoundTypes		m_eSoundHide;
    ESoundTypes		m_eSoundShot;
    ESoundTypes		m_eSoundEmptyClick;
    ESoundTypes		m_eSoundReload;
#ifdef NEW_SOUNDS //AVO: new sounds go here
    ESoundTypes		m_eSoundReloadEmpty;
    ESoundTypes		m_eSoundReloadMisfire;
#endif //-NEW_SOUNDS
    bool			m_sounds_enabled;
    // General
    //кадр момента пересчета UpdateSounds
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
	virtual DLL_Pure*_construct();

    virtual void	Load(LPCSTR section);
	virtual	void	OnEvent(NET_Packet& P, u16 type);

    void	LoadSilencerKoeffs(LPCSTR sect);
    virtual CWeaponMagazined*cast_weapon_magazined()
    {
        return this;
    }

    virtual void	SetDefaults();
    virtual void	FireStart();
    virtual void	FireEnd();
    virtual void	Reload();
	virtual void	StartReload(CObject* to_reload = NULL);

    virtual	void	UpdateCL();
    virtual void	net_Destroy();
    virtual void	net_Export(NET_Packet& P);
    virtual void	net_Import(NET_Packet& P);

    virtual void	OnH_A_Chield();

    virtual bool	Action			(u16 cmd, u32 flags);
    bool			IsAmmoAvailable	();

    virtual bool	GetBriefInfo(II_BriefInfo& info);

	virtual	xr_vector<CCartridge>&		Magazine();
			u32							MagazineElapsed();

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
    //максимальный размер очереди, которой можно стрельнуть
    int				m_iQueueSize;
    //количество реально выстреляных патронов
    int				m_iShotNum;
    //после какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-за Абакана)
    int				m_iBaseDispersionedBulletsCount;
    //скорость вылета патронов, на которые не влияет отдача (сделано из-за Абакана)
    float			m_fBaseDispersionedBulletsSpeed;
    //скорость вылета остальных патронов
    float			m_fOldBulletSpeed;
    Fvector			m_vStartPos, m_vStartDir;
    //флаг того, что мы остановились после того как выстреляли
    //ровно столько патронов, сколько было задано в m_iQueueSize
    bool			m_bStopedAfterQueueFired;
    //флаг того, что хотя бы один выстрел мы должны сделать
    //(даже если очень быстро нажали на курок и вызвалось FireEnd)
    bool			m_bFireSingleShot;
    //режимы стрельбы
    bool			m_bHasDifferentFireModes;
    xr_vector<s8>	m_aFireModes;
    int				m_iCurFireMode;
    int				m_iPrefferedFireMode;

    //переменная блокирует использование
    //только разных типов патронов
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

    //виртуальные функции для проигрывания анимации HUD
    virtual void	PlayAnimShow();
    virtual void	PlayAnimHide();
    virtual void	PlayAnimReload();
    virtual void	PlayAnimIdle();
    virtual void	PlayAnimShoot();
    virtual void	PlayReloadSound();
	virtual void	PlayAnimAim();
	virtual void	PlayAnimBore();

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

//xMod ported
public:
	   void                                     UpdateSecondVP                           () const;

//xMod altered
public:
	virtual bool                                need_renderable                          ();
	virtual bool                                render_item_ui_query                     ();
	virtual void                                render_item_ui                           ();
	virtual void                                ZoomInc                                  ();
	virtual void                                ZoomDec                                  ();
	virtual void                                modify_holder_params                     (float &range, float &fov) const;
	virtual void                                renderable_Render                        ();
	virtual void                                render_hud_mode                          ();
	virtual float                               Weight                                   () const;
	virtual float                               Volume                                   () const;

//xMod added
private:
	   u8                                       m_iChamber;
	   bool                                     m_bIronSightsLowered;

	CScope*										m_pScope;
	CScope*										m_pAltScope;
	CSilencer*									m_pSilencer;
	CMagazineObject*							m_pMagazine;

	   CMagazineObject*                         m_pMagazineToReload;
	

	   void                                     LoadCartridgeFromMagazine                (bool set_ammo_type_only = false);
	   void                                     UpdateSndShot                            ();

	   CScope*                                  GetActiveScope                           () const;

	virtual float                               GetControlInertionFactorBase             () const;

	virtual void                                PrepareCartridgeToShoot                  ();
	virtual void                                ConsumeShotCartridge                     ();

protected:
	CWeaponAmmo*								m_pCartridgeToReload;

	void									V$	ProcessAddon							(CAddon CPC addon, BOOL attach, SAddonSlot CPC slot);

public:
	bool										Discharge								(CCartridge& destination);

	bool										ScopeAttached							()	C$	{ return m_pScope || m_pAltScope; }
	bool										SilencerAttached						()	C$	{ return !!m_pSilencer; }

	float										GetLensRotatingFactor					()	C$;
	float										GetReticleScale							()	C$;
	bool										CanTrade								()	C$;

	float										CurrentZoomFactor						()	CO$;

	void										UpdateBonesVisibility                    ()	O$;
	void										UpdateHudBonesVisibility                 ()	O$;
	void										UpdateAddonsTransform                    ()	O$;
	void										TransferAnimation                        (CAddonObject* addon, bool attach) O$;
	void										OnTaken                                  ()	O$;

	bool									V$	LoadCartridge                            (CWeaponAmmo* cartridges);
	void									V$	OnMotionHalf                             ();
};
