#pragma once

#include "../xrphysics/PhysicsShell.h"
#include "weaponammo.h"
#include "PHShellCreator.h"

#include "ShootingObject.h"
#include "inventory_item_object.h"
#include "Actor_Flags.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "game_cl_single.h"
#include "first_bullet_controller.h"

#include "actor.h"

class CEntity;
class ENGINE_API CMotionDef;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemWeaponAmmo;
class CWeaponMagazined;
class CParticlesObject;

struct SafemodeAnm
{
	LPCSTR name;
	float power, speed;
};

class CWeapon : public CInventoryItemObject,
	public CShootingObject
{
private:
	typedef CInventoryItemObject inherited;

public:
	CWeapon();
	virtual					~CWeapon();

	// Generic
	virtual void			Load(LPCSTR section);

	virtual BOOL			net_Spawn(CSE_Abstract* DC);
	virtual void			net_Destroy();

	virtual CWeapon			*cast_weapon()
	{
		return this;
	}
	virtual CWeaponMagazined*cast_weapon_magazined()
	{
		return 0;
	}

	//serialization
	virtual BOOL			net_SaveRelevant()
	{
		return inherited::net_SaveRelevant();
	}

	virtual void			UpdateCL();

	virtual void			renderable_Render();
	virtual void			render_hud_mode();
	
			void			OnH_A_Chield() override;
			void			OnH_B_Chield() override;
	virtual void			OnH_B_Independent(bool just_before_destroy);
	virtual void			OnH_A_Independent();

	virtual	void			Hit(SHit* pHDS);

	virtual void			reinit();
	virtual void			reload(LPCSTR section);
	virtual void			create_physic_shell();
	virtual void			activate_physic_shell();
	virtual void			setup_physic_shell();

	virtual void			OnActiveItem();
	virtual void			OnHiddenItem();

public:
	virtual bool			can_kill() const;
	virtual CInventoryItem	*can_kill(CInventory *inventory) const;
	virtual const CInventoryItem *can_kill(const xr_vector<const CGameObject*> &items) const;
	virtual bool			ready_to_kill() const;
	virtual ALife::_TIME_ID	TimePassedAfterIndependant() const;
protected:
	//время удаления оружия
	ALife::_TIME_ID			m_dwWeaponRemoveTime;
	ALife::_TIME_ID			m_dwWeaponIndependencyTime;

	virtual bool			IsHudModeNow();
public:
	void					signal_HideComplete();
	virtual bool			Action(u16 cmd, u32 flags);

	enum EWeaponStates
	{
		eFire = eLastBaseState + 1,
		eFire2,
		eReload,
		eMisfire,
		eSwitch,
	};
	enum EWeaponSubStates
	{
		eSubstateReloadBegin = 0,
		eSubstateReloadInProcess,
		eSubstateReloadEnd,
		eSubstateReloadDetach,
		eSubstateReloadAttach,
		eSubstateReloadBolt,
		eSubstateReloadChamber,
		eSubstateReloadAttachG,
		eSubstateReloadDetachG,
		eSubstateReloadBoltLock,
	};

	// Does weapon need's update?
	BOOL					IsUpdating();

	BOOL					IsMisfire() const;
	BOOL					CheckForMisfire();

	BOOL					AutoSpawnAmmo() const
	{
		return m_bAutoSpawnAmmo;
	};
	bool					IsTriStateReload() const
	{
		return m_bTriStateReload;
	}
	EWeaponSubStates		GetReloadState() const
	{
		return (EWeaponSubStates) m_sub_state;
	}
protected:
	bool					m_bTriStateReload;
   
	// a misfire happens, you'll need to rearm weapon
	bool					bMisfire;

	BOOL					m_bAutoSpawnAmmo;
	virtual bool			AllowBore();
public:
	u8						m_sub_state;

	IC void	ForceUpdateAmmo()
	{
		m_BriefInfo_CalcFrame = 0;
	}

	float	GetBulletSpeed		() const		{ return CShootingObject::GetBulletSpeed(); }
	float	GetRPM				() const		{ return 60.f / fOneShotTime; }

protected:
	struct SZoomParams
	{
		bool			m_bZoomEnabled;			//разрешение режима приближения
		bool			m_bHideCrosshairInZoom;
		bool			m_bIsZoomModeNow;		//когда режим приближения включен
	} m_zoom_params;

public:
	IC bool					IsZoomEnabled()	const { return m_zoom_params.m_bZoomEnabled; }
	virtual	void			ZoomInc();
	virtual	void			ZoomDec();
	virtual void			OnZoomIn();
	virtual void			OnZoomOut();
	IC		bool			IsZoomed()	const { return m_zoom_params.m_bIsZoomModeNow; };

	bool			ZoomHideCrosshair()
	{
		CActor *pA = smart_cast<CActor *>(H_Parent());
		if (pA && pA->active_cam() == eacLookAt)
			return false;
		return m_zoom_params.m_bHideCrosshairInZoom;
	}

	virtual float			CurrentZoomFactor	(bool for_actor) const;

	virtual EHandDependence		HandDependence()	const
	{
		return eHandDependence;
	}
	bool				IsSingleHanded()	const
	{
		return m_bIsSingleHanded;
	}

public:
	IC		LPCSTR			strap_bone0() const
	{
		return m_strap_bone0;
	}
	IC		LPCSTR			strap_bone1() const
	{
		return m_strap_bone1;
	}
	IC		void			strapped_mode(bool value)
	{
		m_strapped_mode = value;
	}
	IC		bool			strapped_mode() const
	{
		return m_strapped_mode;
	}

protected:
	LPCSTR					m_strap_bone0;
	LPCSTR					m_strap_bone1;
	Fmatrix					m_StrapOffset;
	bool					m_strapped_mode;
	bool					m_can_be_strapped;

	Fmatrix					m_Offset;
	// 0-используется без участия рук, 1-одна рука, 2-две руки
	EHandDependence			eHandDependence;
	bool					m_bIsSingleHanded;

private:
	Fvector					vLastFP = vZero;
	Fvector					vLastFD = vZero;
	Fvector					vLastSP = vZero;
	Fmatrix					m_FireParticlesXForm = Fidentity;

protected:
	virtual void			UpdateFireDependencies_internal();
	virtual void			UpdatePosition(const Fmatrix& transform);	//.
	virtual void			UpdateXForm();
	IC		void			UpdateFireDependencies()
	{
		if (dwFP_Frame == Device.dwFrame) return; UpdateFireDependencies_internal(); dwFP_Frame = Device.dwFrame;
	};
	
public:
	IC		const Fvector&	get_LastFP()
	{
		UpdateFireDependencies(); return vLastFP;
	}
	IC		const Fvector&	get_LastFD()
	{
		UpdateFireDependencies(); return vLastFD;
	}
	IC		const Fvector&	get_LastSP()
	{
		UpdateFireDependencies(); return vLastSP;
	}

	virtual const Fvector&	get_CurrentFirePoint()
	{
		return get_LastFP();
	}
	virtual const Fmatrix&	get_ParticlesXFORM()
	{
		UpdateFireDependencies(); return m_FireParticlesXForm;
	}
	virtual void			ForceUpdateFireParticles();
	virtual void			debug_draw_firedeps();

protected:
	virtual void			SetDefaults();

	virtual bool			MovingAnimAllowedNow();
	virtual void			OnStateSwitch(u32 S, u32 oldState);

	//трассирование полета пули
	virtual	void			FireTrace();
	virtual float			GetWeaponDeterioration();

	virtual void			FireStart()
	{
		CShootingObject::FireStart();
	}
	virtual void			FireEnd();

	void			StopShooting();

	// обработка визуализации выстрела
	virtual void			OnShot()
	{};
	virtual void			AddShotEffector();
	virtual void			RemoveShotEffector();
	virtual	void			ClearShotEffector();
	virtual	void			StopShotEffector();

public:
	float					GetFireDispersion(CCartridge* cartridge);
	float					GetFireDispersion(float cartridge_k);
	virtual	int				ShotsFired()
	{
		return 0;
	}
	virtual	int				GetCurrentFireMode()
	{
		return 1;
	}

	//параметы оружия в зависимоти от его состояния исправности
	float					GetConditionDispersionFactor() const;
	float					GetConditionMisfireProbability() const;
	virtual	float			GetConditionToShow() const;

protected:
	//фактор увеличения дисперсии при максимальной изношености
	//(на сколько процентов увеличится дисперсия)
	float					fireDispersionConditionFactor;
	//вероятность осечки при максимальной изношености

	// modified by Peacemaker [17.10.08]
	//	float					misfireProbability;
	//	float					misfireConditionK;
	float misfireStartCondition;			//изношенность, при которой появляется шанс осечки
	float misfireEndCondition;				//изношеность при которой шанс осечки становится константным
	float misfireStartProbability;			//шанс осечки при изношености больше чем misfireStartCondition
	float misfireEndProbability;			//шанс осечки при изношености больше чем misfireEndCondition
	float conditionDecreasePerQueueShot;	//увеличение изношености при выстреле очередью
	float conditionDecreasePerShot;			//увеличение изношености при одиночном выстреле

public:
	float GetMisfireStartCondition() const
	{
		return misfireStartCondition;
	};
	float GetMisfireEndCondition() const
	{
		return misfireEndCondition;
	};

protected:
	first_bullet_controller	m_first_bullet_controller;

protected:
	int						GetAmmoCount(u8 ammo_type) const;

public:
	virtual int				GetAmmoElapsed()	const
	{
		return m_chamber.size() + m_magazin.size();
	}
	IC int					GetAmmoMagSize()	const
	{
		return m_chamber.capacity() + m_magazin.capacity();
	}
	int						GetSuitableAmmoTotal	(bool use_item_to_spawn = false) const;

	void					SetAmmoElapsed			(int ammo_count);

	virtual void			OnMagazineEmpty			() {}

	float			GetFirstBulletDisp()	const
	{
		return m_first_bullet_controller.get_fire_dispertion();
	};
protected:

	//для подсчета в GetSuitableAmmoTotal
	mutable int				m_iAmmoCurrentTotal;
	mutable u32				m_BriefInfo_CalcFrame;	//кадр на котором просчитали кол-во патронов

	virtual bool			IsNecessaryItem(const shared_str& item_sect);

public:
	xr_vector<shared_str>	m_ammoTypes;

	bool					unlimited_ammo() const;
	IC	bool				can_be_strapped() const
	{
		return m_can_be_strapped;
	};

protected:
	u32						m_ef_main_weapon_type;
	u32						m_ef_weapon_type;

public:
	virtual u32				ef_main_weapon_type() const;
	virtual u32				ef_weapon_type() const;

	//Alundaio
	int						GetAmmoCount_forType(shared_str const& ammo_type) const;
	virtual void			set_ef_main_weapon_type(u32 type){ m_ef_main_weapon_type = type; };
	virtual void			set_ef_weapon_type(u32 type){ m_ef_weapon_type = type; };
	//-Alundaio

public:
	virtual bool			use_crosshair()	const { return true; }
	bool					show_crosshair();
	bool					show_indicators();
	virtual BOOL			ParentMayHaveAimBullet();

private:
	bool					process_if_exists_deg2rad		(LPCSTR section, LPCSTR name, float& value, bool test);

protected:
	virtual bool			install_upgrade_impl(LPCSTR section, bool test);

private:
	float					m_hit_probability[egdCount];

public:
	const float				&hit_probability() const;

private:
	Fvector					m_overriden_activation_speed;
	bool					m_activation_speed_is_overriden;
	virtual bool			ActivationSpeedOverriden(Fvector& dest, bool clear_override);

public:
	virtual void			SetActivationSpeedOverride(Fvector const& speed);

	virtual shared_str const	GetAnticheatSectionName() const
	{
		return cNameSect();
	};

private:
	static float						s_inertion_baseline_weight;
	static float						s_inertion_ads_factor;
	static float						s_inertion_aim_factor;
	static float						s_inertion_armed_factor;
	static float						s_inertion_relaxed_factor;

	static float						s_recoil_kick_weight;
	static float						s_recoil_tremble_weight;
	static float						s_recoil_roll_weight;
	static float						s_recoil_tremble_mean_change_chance;
	static float						s_recoil_tremble_dispersion;
	static float						s_recoil_kick_dispersion;
	static float						s_recoil_roll_dispersion;

	SafemodeAnm							m_safemode_anm[2];

protected:
	int									m_iADS									= 0;
	bool								m_bArmedMode							= false;
	bool								m_bHasAltAim							= true;
	bool								m_bArmedRelaxedSwitch					= true;
	bool								m_locked								= false;
	CActor*								m_actor									= nullptr;

	float								m_grip_accuracy_modifier				= 1.f;
	float								m_stock_accuracy_modifier				= 1.f;
	float								m_layout_accuracy_modifier				= 1.f;

	Fvector								m_stock_recoil_pattern					= vOne;
	Fvector								m_layout_recoil_pattern					= vOne;
	Fvector								m_mechanic_recoil_pattern				= vOne;

	float								m_recoil_tremble_mean					= 0.f;
	Fvector4							m_recoil_hud_impulse					= vZero4;
	Fvector4							m_recoil_hud_shift						= vZero4;
	Fvector								m_recoil_cam_impulse					= vZero;
	Fvector								m_recoil_cam_delta						= vZero;
	Fvector								m_recoil_cam_last_impulse				= vZero;
	
	Fvector								m_root_bone_position					= vZero;
	Fvector								m_loaded_muzzle_point					= vZero;
	Fvector								m_muzzle_point							= vZero;
	Fvector								m_shell_point							= vZero;
	u16									m_shell_bone							= 0;
	
	xr_vector<CCartridge>				m_chamber								= {};
	xr_vector<CCartridge>				m_magazin								= {};
	
	CCartridge							m_cartridge;
	
	void								appendRecoil							(float impulse_magnitude);
	
	float								readAccuracyModifier				C$	(LPCSTR section, LPCSTR line);
	Fvector								readRecoilPattern					C$	(LPCSTR section, LPCSTR line);
	int									get_ammo_type						C$	(shared_str CR$ section);
	
	float								Aboba								O$	(EEventTypes type, void* data, int param);

	//with zeroing
	Fvector							V$	getFullFireDirection					(CCartridge CR$ c)		{ return get_LastFD(); }
	CCartridge						V$	getCartridgeToShoot						()						{ return m_chamber.back(); }
	bool							V$	HasAltAim							C$	()						{ return m_bHasAltAim; }
	
	void							V$	SetADS									(int mode);

public:
	static void							loadStaticVariables						();

	void								SwitchArmedMode							();

	Fvector4 CR$ 						getRecoilHudShift					C$	()		{ return m_recoil_hud_shift; }
	Fvector CR$							getRecoilCamDelta					C$	()		{ return m_recoil_cam_delta; }
	bool								ArmedMode							C$	()		{ return m_bArmedMode; }
	
	bool								isCamRecoilRelaxed					C$	();
	float								GetControlInertionFactor			CO$	();
	bool								NeedBlendAnm						O$	();
	
	int								V$	ADS									C$	();
};
