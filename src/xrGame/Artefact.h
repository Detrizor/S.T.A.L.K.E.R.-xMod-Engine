#pragma once

#include "inventory_item_object.h"
#include "hit_immunity.h"
#include "../xrphysics/PHUpdateObject.h"
#include "script_export_space.h"
#include "patrol_path.h"
#include "item_amountable.h"

class SArtefactActivation;
struct SArtefactDetectorsSupport;

class CArtefact :	public CInventoryItemObject,
	public CPHUpdateObject
{
	typedef CInventoryItemObject inherited;

public:
									CArtefact						();

	virtual void					Load							(LPCSTR section);
	virtual BOOL					net_Spawn						(CSE_Abstract* DC);
	virtual void					net_Destroy						();

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
	float								m_baseline_charge						= 1.f;
	float								m_charge_capacity						= 1.f;
	float								m_saturation_power						= 1.f;
	float								m_baseline_weight_dump					= 0.f;
	float								m_armor									= 0.f;
	float								m_HitAbsorbation[ALife::eHitTypeMax]	= {0};

	MAmountable CP$						m_amountable_ptr						= nullptr;
	mutable u32							m_power_calc_frame						= 0;
	mutable float						m_power									= 1.f;
	mutable float						m_radiation								= 1.f;

protected:
	void								Hit									O$	(SHit* pHDS);

public:
	float								getArmor							C$	()					{ return m_armor * getPower(); }
	float								getAbsorbation						C$	(int hit_type)		{ return m_HitAbsorbation[hit_type] * getPower(); }
	float								getPower							C$	()					{ updatePower(); return m_power; }
	float								getRadiation						C$	()					{ updatePower(); return m_radiation; }

	void								updatePower							C$	();
	float								HitProtection						C$	(ALife::EHitType hit_type);
	float								getWeightDump						C$	();

	void								ProcessHit								(float d_damage, ALife::EHitType hit_type);
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
