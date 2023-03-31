#pragma once

#include "inventory_item_object.h"
#include "hit_immunity.h"
#include "../xrphysics/PHUpdateObject.h"
#include "script_export_space.h"
#include "patrol_path.h"
#include "inventory_item_amountable.h"

class SArtefactActivation;
struct SArtefactDetectorsSupport;

class CArtefact :	public CInventoryItemObject,
	public CPHUpdateObject,
	public CItemAmountable
{
	typedef CInventoryItemObject inherited;

public:
									CArtefact						();
	virtual							~CArtefact						();
	virtual DLL_Pure*				_construct						();

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
	float							m_fArmor;
	float							m_fChargeThreshold;
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
	float 							m_fRadiationRestoreSpeed;
	float							m_fWeightDump;
	float							m_HitAbsorbation[ALife::eHitTypeMax];

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

public:
			float			GetArmor				()								const	{ return m_fArmor * Power(); }
			bool			IsActive				()								const	{ return m_bActive; }
			void			SetActive				(bool status)							{ m_bActive = status; }
			float			HitAbsorbation			(ALife::EHitType hit_type)		const	{ return m_HitAbsorbation[hit_type] * Power(); }

			float			Power					()								const;
			float			HitProtection			(ALife::EHitType hit_type)		const;

			void			ProcessHit				(float d_damage, ALife::EHitType hit_type);
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
