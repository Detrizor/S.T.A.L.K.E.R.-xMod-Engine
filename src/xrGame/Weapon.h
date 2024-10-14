#pragma once

#include "../xrphysics/PhysicsShell.h"
#include "weaponammo.h"
#include "PHShellCreator.h"

#include "ShootingObject.h"
#include "inventory_item_object.h"
#include "Actor_Flags.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "game_cl_single.h"

#include "actor.h"

class CEntity;
class ENGINE_API CMotionDef;
class CSE_ALifeItemWeapon;
class CWeaponMagazined;
class CParticlesObject;

class CWeapon : public CInventoryItemObject,
	public CShootingObject
{
private:
	typedef CInventoryItemObject inherited;

public:
	CWeapon();

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

	virtual void			reload(LPCSTR section);
	virtual void			create_physic_shell();
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
	//����� �������� ������
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
		eFiremode
	};
	enum EWeaponSubStates
	{
		eSubstateReloadBegin = 0,
		eSubstateReloadInProcess,
		eSubstateReloadEnd,
		eSubstateReloadAttach,
		eSubstateReloadDetach,
		eSubstateReloadBoltPull,
		eSubstateReloadBoltLock,
		eSubstateReloadBoltRelease,
		eSubstateReloadAttachG,
		eSubstateReloadDetachG,
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
		if (m_sub_state <= eSubstateReloadEnd && !m_bTriStateReload)
			return eSubstateReloadBegin;
		return static_cast<EWeaponSubStates>(m_sub_state);
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

	float	GetRPM				() const		{ return m_rpm; }

protected:
	struct SZoomParams
	{
		bool			m_bZoomEnabled;			//���������� ������ �����������
		bool			m_bIsZoomModeNow;		//����� ����� ����������� �������
	} m_zoom_params;

public:
	IC bool					IsZoomEnabled()	const { return m_zoom_params.m_bZoomEnabled; }
	virtual	void			ZoomInc();
	virtual	void			ZoomDec();
	IC		bool			IsZoomed()	const { return m_zoom_params.m_bIsZoomModeNow; };

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
	// 0-������������ ��� ������� ���, 1-���� ����, 2-��� ����
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

	virtual void			OnStateSwitch(u32 S, u32 oldState);

	//������������� ������ ����
	virtual	void			FireTrace();
	virtual float			GetWeaponDeterioration();

	virtual void			FireStart()
	{
		CShootingObject::FireStart();
	}
	virtual void			FireEnd();

	void			StopShooting();

	// ��������� ������������ ��������
	virtual void			OnShot()
	{};
	virtual void			AddShotEffector();
	virtual void			RemoveShotEffector();
	virtual	void			ClearShotEffector();
	virtual	void			StopShotEffector();

public:
	float					GetFireDispersion();
	float					GetFireDispersion(float cartridge_k);
	virtual	int				ShotsFired()
	{
		return 0;
	}
	virtual	int				GetCurrentFireMode()
	{
		return 1;
	}

	//�������� ������ � ���������� �� ��� ��������� �����������
	float					GetConditionDispersionFactor() const;
	float					GetConditionMisfireProbability() const;
	virtual	float			GetConditionToShow() const;

protected:
	//������ ���������� ��������� ��� ������������ �����������
	//(�� ������� ��������� ���������� ���������)
	float					fireDispersionConditionFactor;
	//����������� ������ ��� ������������ �����������

	// modified by Peacemaker [17.10.08]
	//	float					misfireProbability;
	//	float					misfireConditionK;
	float misfireStartCondition;			//������������, ��� ������� ���������� ���� ������
	float misfireEndCondition;				//����������� ��� ������� ���� ������ ���������� �����������
	float misfireStartProbability;			//���� ������ ��� ����������� ������ ��� misfireStartCondition
	float misfireEndProbability;			//���� ������ ��� ����������� ������ ��� misfireEndCondition
	float conditionDecreasePerQueueShot;	//���������� ����������� ��� �������� ��������
	float conditionDecreasePerShot;			//���������� ����������� ��� ��������� ��������

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
	int						GetAmmoCount(u8 ammo_type) const;

public:
	virtual int				GetAmmoElapsed			()	const { return 0; }
	virtual int				GetAmmoMagSize			()	const { return 0; }
	int						GetSuitableAmmoTotal	() const;
	virtual void			OnMagazineEmpty			() {}

protected:
	//��� �������� � GetSuitableAmmoTotal
	mutable int				m_iAmmoCurrentTotal;
	mutable u32				m_BriefInfo_CalcFrame;	//���� �� ������� ���������� ���-�� ��������

	virtual bool			IsNecessaryItem(const shared_str& item_sect);

public:
	xr_vector<shared_str>	m_ammoTypes{};

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
	bool					show_crosshair() const;
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

public:
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
	static float						s_recoil_tremble_mean_dispersion;
	static float						s_recoil_tremble_mean_change_chance;
	static float						s_recoil_tremble_dispersion;
	static float						s_recoil_kick_dispersion;
	static float						s_recoil_roll_dispersion;

	static Fvector						m_stock_recoil_pattern_absent;
	
	Fvector								m_mechanic_recoil_pattern;
	Fvector								m_layout_recoil_pattern;
	SScriptAnm							m_safemode_anm[2];

protected:
	static float						readAccuracyModifier					(LPCSTR section, LPCSTR line);
	static float						readAccuracyModifier					(LPCSTR type);

	int									m_iADS									= 0;
	bool								m_bHasAltAim							= true;
	bool								m_bArmedRelaxedSwitch					= true;
	CActor*								m_actor									= nullptr;

	float								m_grip_accuracy_modifier				= 1.f;
	float								m_stock_accuracy_modifier				= 1.f;
	float								m_layout_accuracy_modifier				= 1.f;
	float								m_foregrip_accuracy_modifier			= 1.f;
	
	Fvector								m_stock_recoil_pattern					= vOne;
	Fvector								m_muzzle_recoil_pattern					= vOne;
	Fvector								m_foregrip_recoil_pattern				= vOne;

	float								m_recoil_tremble_mean					= 0.f;
	Fvector4							m_recoil_hud_impulse					= vZero4;
	Fvector4							m_recoil_hud_shift						= vZero4;
	Fvector								m_recoil_cam_impulse					= vZero;
	Fvector								m_recoil_cam_delta						= vZero;
	Fvector								m_recoil_cam_last_impulse				= vZero;

	Fvector								m_grip_point							= vZero;
	Fvector								m_fire_point							= vZero;
	Fvector								m_shell_point							= vZero;
	u16									m_shell_bone							= u16_max;
	u16									m_fire_bone								= u16_max;
	
	void								appendRecoil							(float impulse_magnitude);
	float								get_wpn_pos_inertion_factor			C$	();

	float								Aboba								O$	(EEventTypes type, void* data, int param);

	//with zeroing
	virtual Fvector						getFullFireDirection					(CCartridge CR$ c)		{ return get_LastFD(); }
	virtual void						prepare_cartridge_to_shoot				()						{}

	virtual bool						HasAltAim							C$	()						{ return m_bHasAltAim; }
	
	virtual void						setADS									(int mode);
	virtual void						setAiming								(bool mode);

public:
	static void							loadStaticData							();
	static Fvector						readRecoilPattern						(LPCSTR section, LPCSTR line);
	static Fvector						readRecoilPattern						(LPCSTR type);

	void								SwitchArmedMode							();

	Fvector4 CR$ 						getRecoilHudShift					C$	()		{ return m_recoil_hud_shift; }
	Fvector CR$							getRecoilCamDelta					C$	()		{ return m_recoil_cam_delta; }
	Fmatrix CR$							getOffset							C$	()		{ return m_Offset; }
	Fvector CR$							getLoadedFirePoint					C$	()		{ return m_fire_point; }
	
	bool								isCamRecoilRelaxed					C$	();
	bool								ArmedMode							C$	();

	float								GetControlInertionFactor			CO$	(bool full = false);
	
	int								V$	ADS									C$	()		{ return m_iADS; }
};
