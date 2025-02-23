﻿#pragma once

class CSE_Abstract;
class CPhysicItem;
class NET_Packet;
class CInventoryItem;
class CMotionDef;

#include "actor_defs.h"
#include "inventory_space.h"
#include "hudsound.h"
#include "script_export_space.h"

struct attachable_hud_item;
class motion_marks;
struct script_layer;

struct SScriptAnm
{
	shared_str							anm										= 0;
	float								power									= 1.f;
	float								speed									= 1.f;

	void								load									(shared_str CR$ section, LPCSTR name);
	bool								loaded								C$	()		{ return anm.size(); }
};

class CHUDState
{
public:
enum EHudStates {
		eIdle		= 0,
		eShowing,
		eHiding,
		eHidden,
		eBore,
		eLastBaseState = eBore,
};

private:
	u32						m_hud_item_state;
	u32						m_dw_curr_state_time;

protected:
	u32						m_dw_curr_substate_time;

public:
							CHUDState			() 						{SetState(eHidden);}
	IC		u32				GetState			() const				{return		m_hud_item_state;}
	
	IC		void			SetState			(u32 v)					{m_hud_item_state = v; m_dw_curr_state_time=Device.dwTimeGlobal;ResetSubStateTime();}
	IC		u32				CurrStateTime		() const				{return Device.dwTimeGlobal-m_dw_curr_state_time;}
	IC		void			ResetSubStateTime	()						{m_dw_curr_substate_time=Device.dwTimeGlobal;}
	virtual void			SwitchState			(u32 S)					= 0;
	virtual void			OnStateSwitch		(u32 S, u32 oldState) 	= 0;
};

class CHudItem :public CHUDState
{
public:
							CHudItem			();
	virtual DLL_Pure*		_construct			();
protected:
	Flags16					m_huditem_flags;
	enum{
		fl_pending			= (1<<0),
		fl_renderhud		= (1<<1),
		fl_inertion_enable	= (1<<2),
		fl_inertion_allow	= (1<<3),
	};

	struct{
		const CMotionDef*		m_current_motion_def;
		shared_str				m_current_motion;
		u32						m_dwMotionCurrTm;
		u32						m_dwMotionStartTm;
		u32						m_dwMotionEndTm;
		u32						m_startedMotionState;
		u8						m_started_rnd_anim_idx;
		bool					m_bStopAtEndAnimIsRunning;
	};

public:
	virtual void				Load				(LPCSTR section);
	virtual	BOOL				net_Spawn			(CSE_Abstract* DC)				{return TRUE;};
	virtual void				net_Destroy			()								{};

	virtual void				OnH_A_Chield		();
	virtual void				OnH_B_Chield		();
	virtual void				OnH_B_Independent	(bool just_before_destroy);
	virtual void				OnH_A_Independent	();
	
	virtual void				PlaySound			(LPCSTR alias, const Fvector& position);
	virtual void				PlaySound			(LPCSTR alias, const Fvector& position, u8 index); //Alundaio: Play at index

	virtual bool				Action				(u16 cmd, u32 flags)			{return false;}

	BOOL						GetHUDmode			();
	IC BOOL						IsPending			()		const					{ return !!m_huditem_flags.test(fl_pending);}

	bool						activateItem		(u16 prev_slot = u16_max);
	void						deactivateItem		(u16 slot = u16_max);
	void						hideItem			();
	void						restoreItem			();

	virtual void				OnActiveItem		();
	virtual void				OnHiddenItem		()				{ SwitchState(eHiding); }
	virtual void				OnMoveToRuck		(const SInvItemPlace& prev);

	bool						IsHidden			()	const		{	return GetState() == eHidden;}						// Does weapon is in hidden state
	bool						IsHiding			()	const		{	return GetState() == eHiding;}
	bool						IsShowing			()	const		{	return GetState() == eShowing;}

	virtual void				SwitchState			(u32 S);
	virtual void				OnStateSwitch		(u32 S, u32 oldState);

	virtual void				OnAnimationEnd		(u32 state);
	virtual void				OnMotionMark		(u32 state, const motion_marks&){};

	virtual void				PlayAnimIdle		();
	virtual void				PlayAnimBore		();

	virtual void				UpdateCL			();

	virtual void				UpdateHudAdditional	(Dmatrix&);

	virtual	void				UpdateXForm			()						= 0;
	virtual void				activate_physic_shell();
	virtual void				on_activate_physic_shell() { R_ASSERT2(0, "failed call of virtual function!"); }

	u32							PlayHUDMotion		(shared_str name, BOOL bMixIn, u32 state, bool ignore_anm_type = false);
	u32							PlayHUDMotion_noCB	(const shared_str& M, BOOL bMixIn, bool ignore_anm_type);
	void						StopCurrentAnimWithoutCallback();

	IC void						RenderHud				(BOOL B)	{ m_huditem_flags.set(fl_renderhud, B);}
	IC BOOL						RenderHud				()			{ return m_huditem_flags.test(fl_renderhud);}
	attachable_hud_item*		HudItemData				() const;
	virtual void				on_a_hud_attach			();
	virtual void				on_b_hud_detach			();
	IC BOOL						HudInertionEnabled		()	const			{ return m_huditem_flags.test(fl_inertion_enable);}
	IC BOOL						HudInertionAllowed		()	const			{ return m_huditem_flags.test(fl_inertion_allow);}
	virtual void				render_hud_mode			();
	virtual bool				need_renderable			()					{return true;};
	virtual void				render_item_3d_ui		()					{}
	virtual bool				render_item_3d_ui_query	()					{return false;}

	virtual bool				CheckCompatibility		(CHudItem*)			{return true;}

protected:

	IC		void				SetPending			(BOOL H)			{ m_huditem_flags.set(fl_pending, H);}
	shared_str					hud_sect;

	//кадры момента пересчета XFORM и FirePos
	u32							dwFP_Frame;
	u32							dwXF_Frame;

	IC void						EnableHudInertion		(BOOL B)		{ m_huditem_flags.set(fl_inertion_enable, B);}
	IC void						AllowHudInertion		(BOOL B)		{ m_huditem_flags.set(fl_inertion_allow, B);}

	u32							m_animation_slot;

	HUD_SOUND_COLLECTION		m_sounds;

private:
	CPhysicItem					*m_object;
	CInventoryItem				*m_item;

public:
	const shared_str&			HudSection				() const		{ return hud_sect;}
	IC CPhysicItem&				object					() const		{ VERIFY(m_object); return(*m_object);}
	IC CInventoryItem&			item					() const		{ VERIFY(m_item); return(*m_item);}
	virtual	u32					animation_slot			() const		{ return m_animation_slot;}

	virtual CHudItem*			cast_hud_item			()				{ return this; }
	bool HudAnimationExist(LPCSTR anim_name);

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	SScriptAnm							m_show_anm;
	SScriptAnm							m_hide_anm;
	SScriptAnm							m_draw_anm_primary;
	SScriptAnm							m_draw_anm_secondary;
	SScriptAnm							m_holster_anm_primary;
	SScriptAnm							m_holster_anm_secondary;

	u16									anm_slot								= u16_max;
	shared_str							m_current_anm							= 0;

protected:
	float								m_fLR_CameraFactor; // Фактор бокового наклона худа при ходьбе [-1; +1]
	float								m_fLR_MovingFactor; // Фактор бокового наклона худа при движении камеры [-1; +1]
	float								m_fLR_InertiaFactor; // Фактор горизонтальной инерции худа при движении камеры [-1; +1]
	float								m_fUD_InertiaFactor; // Фактор вертикальной инерции худа при движении камеры [-1; +1]

	shared_str							m_anm_prefix							= 0;

	void								playBlendAnm							(SScriptAnm CR$ anm, u32 state = 0, bool full_blend = false, float power_k = 1.f);

	LPCSTR							V$	get_anm_prefix						C$	()		{ return *m_anm_prefix; }

public:
	bool								motionPartPassed					C$	(float part);

	void								UpdateSlotsTransform					(); // Обновление положения аддонов на худе каждый кадр
	void								UpdateHudBonesVisibility				();

	virtual LPCSTR						anmType								C$	()		{ return ""; }
	virtual void						onMovementChanged						();
};

add_to_type_list(CHudItem)
#undef script_type_list
#define script_type_list save_type_list(CHudItem)
