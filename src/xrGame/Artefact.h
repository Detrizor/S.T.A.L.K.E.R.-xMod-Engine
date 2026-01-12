#pragma once

#include "inventory_item_object.h"
#include "hit_immunity.h"
#include "../xrphysics/PHUpdateObject.h"
#include "script_export_space.h"
#include "patrol_path.h"
#include "item_amountable.h"

class SArtefactActivation;
struct SArtefactDetectorsSupport;

class CArtefact : public CInventoryItemObject, public CPHUpdateObject
{
	typedef CInventoryItemObject inherited;

public:
	enum EAbsorbationTypes
	{
		eBurnImmunity,
		eShockImmunity,
		eChemBurnImmunity,
		eRadiationImmunity,
		eTelepaticImmunity,
		eAbsorbationTypeMax
	};

	enum EConditionRestoreTypes
	{
		eRadiationSpeed,
		ePainkillSpeed,
		eRegenerationSpeed,
		eRecuperationSpeed,
		eRestoreTypeMax
	};

	static constexpr LPCSTR strAbsorbationNames[eAbsorbationTypeMax]
	{
		"burn_absorbation",
		"shock_absorbation",
		"chemical_burn_absorbation",
		"radiation_absorbation",
		"telepatic_absorbation"
	};

	static constexpr LPCSTR strRestoreNames[eRestoreTypeMax]
	{
		"radiation_speed",
		"painkill_speed",
		"regeneration_speed",
		"recuperation_speed"
	};

public:
									CArtefact						();

	virtual void					Load							(LPCSTR section);
	virtual BOOL					net_Spawn						(CSE_Abstract* DC);
	virtual void					net_Destroy						();

	bool ActivateItem(u16 prev_slot) override;
	void OnMoveToRuck(const SInvItemPlace& prev) override;

	virtual void					OnH_A_Chield					();
	virtual void					OnH_B_Independent				(bool just_before_destroy);
	
	virtual void					UpdateCL						();
	virtual void					shedule_Update					(u32 dt);	
			void					UpdateWorkload					(u32 dt);

	
	virtual bool					CanTake							() const;

	virtual BOOL					renderable_ShadowGenerate		()		{ return FALSE;	}
	virtual BOOL					renderable_ShadowReceive		()		{ return TRUE;	}
	virtual void					create_physic_shell				();

	virtual CArtefact*				cast_artefact					()		{return this;}                         

protected:
	virtual void					UpdateCLChild					()		{};
	virtual void					CreateArtefactActivation		();

	SArtefactActivation*			m_activationObj;
	SArtefactDetectorsSupport*		m_detectorObj;

	u16								m_CarringBoneID;
	shared_str						m_sParticlesName;

	ref_light						m_pTrailLight;
	Fcolor							m_TrailLightColor;
	float							m_fTrailLightRange;
	u8								m_af_rank;
	bool							m_bLightsEnabled;
	bool							m_bActive;

	virtual void					UpdateLights					();

public:
	IC u8							GetAfRank						() const		{return m_af_rank;}
	IC bool							CanBeActivated					()				{return m_bCanSpawnZone;};
	void							ActivateArtefact				();
	void							FollowByPath					(LPCSTR path_name, int start_idx, Fvector magic_force);
	bool							CanBeInvisible					();
	void							SwitchVisibility				(bool);

	void							SwitchAfParticles				(bool bOn);
	virtual void					StartLights();
	virtual void					StopLights();

	virtual void					PhDataUpdate					(float step);
	virtual void					PhTune							(float step)	{};
	
	bool							m_bCanSpawnZone;

public:
	enum EAFHudStates {
		eActivating = eLastBaseState+1,
	};
	virtual void					Interpolate			();

	virtual	void					PlayAnimIdle		();
	virtual void					MoveTo(Fvector const & position);
	virtual void					StopActivation		();

	virtual void					ForceTransform		(const Fmatrix& m);

	virtual bool					Action				(u16 cmd, u32 flags);
	virtual void					OnStateSwitch		(u32 S, u32 oldState);
	virtual void					OnAnimationEnd		(u32 state);
	virtual bool					IsHidden			()	const	{return GetState()==eHidden;}

	// optimization FAST/SLOW mode
	u32						o_render_frame				;
	BOOL					o_fastmode					;
	IC void					o_switch_2_fast				()	{
		if (o_fastmode)		return	;
		o_fastmode			= TRUE	;
		//processing_activate		();
	}
	IC void					o_switch_2_slow				()	{
		if (!o_fastmode)	return	;
		o_fastmode			= FALSE	;
		//processing_deactivate		();
	}

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	void update_power() const;
	void update_trail_light_color();

public:
	float getPower() const { update_power(); return m_fPower; }
	float getRadiation() const { update_power(); return m_fRadiation; }
	float getArmor() const { return _armor * getPower(); }
	float getAbsorbation(int hit_type) const { return _fHitAbsorbations[hit_type] * getPower(); }
	float getRestore(EConditionRestoreTypes type) const { return _fRestores[type] * getPower(); }
	float getMaxCharge() const { return _maxCharge; }

	float getHitProtection(ALife::EHitType hitType) const;
	float getWeightDump() const;

	void processHit(float fDamage, ALife::EHitType hitType);

	bool canBeDetected() const noexcept;

public:
	void Hit(SHit* pHDS) override;

private:
	MAmountable* _amountable{};

	float _baselineCharge{ 1.F };
	float _weightPart{ 1.F };
	float _maxCharge{ 1.F };
	float _armor{ 0.F };

	bool _overcharge{ false };

	bool _trailLightColorAmountDependant{ false };
	float _trailLightColorAmountEmpty{ 0.F };
	float _trailLightColorAmountMin{ 0.F };
	float _trailLightColorAmountMax{ 0.F };

	xarr<float, ALife::eHitTypeMax> _fHitAbsorbations{};
	xarr<float, eRestoreTypeMax> _fRestores{};

	mutable u32 m_nPowerCalcFrame{ 0 };
	mutable float m_fPower{ 0.F };
	mutable float m_fRadiation{ 0.F };
};

struct SArtefactDetectorsSupport
{
	CArtefact*						m_parent;
	ref_sound						m_sound;

	Fvector							m_path_moving_force;
	u32								m_switchVisTime;
	const CPatrolPath*				m_currPatrolPath;
	const CPatrolPath::CVertex*		m_currPatrolVertex;
	Fvector							m_destPoint;

			SArtefactDetectorsSupport		(CArtefact* A);
			~SArtefactDetectorsSupport		();
	void	SetVisible						(bool);
	void	FollowByPath					(LPCSTR path_name, int start_idx, Fvector force);
	void	UpdateOnFrame					();
	void	Blink							();
};

add_to_type_list(CArtefact)
#undef script_type_list
#define script_type_list save_type_list(CArtefact)
