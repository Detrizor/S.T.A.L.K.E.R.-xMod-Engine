#pragma once
#include "firedeps.h"
#include "../xrEngine/ObjectAnimator.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "actor_defs.h"

class player_hud;
class CHudItem;
class CMotionDef;

struct motion_descr
{
	MotionID		mid;
	shared_str		name;
};

struct player_hud_motion
{
	shared_str				m_alias_name;
	shared_str				m_base_name;
	shared_str				m_additional_name;
	xr_vector<motion_descr>	m_animations;
	float					m_anim_speed;
};

struct player_hud_motion_container
{
	xr_vector<player_hud_motion>	m_anims;
	player_hud_motion*				find_motion(const shared_str& name);
	void		load				(IKinematicsAnimated* model, LPCSTR sect);
};

struct movement_layer
{
	CObjectAnimator* anm;
	float blend_amount[2];
	bool active;
	float m_power;
	Fmatrix blend;
	u8 m_part;

	movement_layer()
	{
		blend.identity();
		anm = xr_new<CObjectAnimator>();
		blend_amount[0] = 0.f;
		blend_amount[1] = 0.f;
		active = false;
		m_power = 1.f;
	}

	void Load(LPCSTR name)
	{
		if (xr_strcmp(name, anm->Name()))
			anm->Load(name);
	}

	void Play(bool bLoop = true)
	{
		if (!anm->Name())
			return;

		if (IsPlaying())
		{
			active = true;
			return;
		}
		
		anm->Play(bLoop);
		active = true;
	}

	bool IsPlaying()
	{
		return anm->IsPlaying();
	}

	void Stop(bool bForce)
	{
		if (bForce)
		{
			anm->Stop();
			blend_amount[0] = 0.f;
			blend_amount[1] = 0.f;
			blend.identity();
		}

		active = false;
	}

	const Fmatrix& XFORM(u8 part)
	{
		blend.set(anm->XFORM());
		blend.mul(blend_amount[part] * m_power);
		blend.m[0][0] = 1.f;
		blend.m[1][1] = 1.f;
		blend.m[2][2] = 1.f;

		return blend;
	}
};

struct script_layer
{
	shared_str m_name;
	CObjectAnimator* anm;
	float blend_amount;
	float m_power;
	bool active;
	Fmatrix blend;
	u8 m_part;

	script_layer(shared_str CR$ name, u8 part, float speed = 1.f, float power = 1.f, bool looped = true, bool full_blend = false)
	{
		m_name = name;
		m_part = part;
		m_power = power;
		anm = xr_new<CObjectAnimator>();
		anm->Load(name.c_str());
		anm->Play(looped);
		anm->Speed() = speed;
		blend_amount = (full_blend) ? 1.f : 0.f;
		active = true;
	}

	bool IsPlaying()
	{
		return anm->IsPlaying();
	}

	void Stop(bool bForce)
	{
		if (bForce)
		{
			anm->Stop();
			blend_amount = 0.f;
		}

		active = false;
	}

	const Fmatrix& XFORM()
	{
		blend.set(anm->XFORM());
		float k = blend_amount * m_power;
		Fvector hpb;
		blend.getHPB(hpb);
		hpb.mul(k);
		Fvector pos = blend.c;
		pos.mul(k);
		blend.setHPBv(hpb);
		blend.translate_over(pos);

		return blend;
	}
};

struct attachable_hud_item;

struct hud_item_measures
{
	enum{e_fire_point=(1<<0), e_fire_point2=(1<<1), e_shell_point=(1<<2)};
	Flags8							m_prop_flags;

	u16								m_fire_bone;
	Fvector							m_fire_point_offset;
	u16								m_fire_bone2;
	Fvector							m_fire_point2_offset;

	void load						(LPCSTR hud_section, IKinematics* K);

	Fvector							m_strafe_offset[4][2]; // pos,rot,data1,data2/ normal,aim-GL	 --#SM+#--

	struct inertion_params
	{
		float m_tendto_speed;
		float m_tendto_speed_aim;
		float m_tendto_ret_speed;
		float m_tendto_ret_speed_aim;

		float m_min_angle;
		float m_min_angle_aim;

		Fvector4 m_offset_LRUD;
		Fvector4 m_offset_LRUD_aim;
	} m_inertion_params; //--#SM+#--
};

struct attachable_hud_item
{
	player_hud*						m_parent;
	CHudItem*						m_parent_hud_item;
	shared_str						m_hud_section;
	shared_str						m_object_section;
	IKinematics*					m_model;
	u16								m_attach_place_idx;
	bool							m_auto_attach;
	hud_item_measures				m_measures;

	//runtime positioning
	Dmatrix							m_item_attach;
	Dmatrix							m_hands_attach;
	Dmatrix							m_transform;

	player_hud_motion_container		m_hand_motions;
			
			attachable_hud_item		(player_hud* pparent):m_parent(pparent),m_upd_firedeps_frame(u32(-1)),m_parent_hud_item(NULL){}
			~attachable_hud_item	();
	void load						(LPCSTR hud_section, LPCSTR object_section);
	void update						(bool bForce);
	void setup_firedeps				(firedeps& fd);
	void render						();	
	void render_item_ui				();
	bool render_item_ui_query		();
	bool need_renderable			();
	void set_bone_visible			(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent=FALSE);
	void debug_draw_firedeps		();

//props
	u32								m_upd_firedeps_frame;
	void		tune				(Ivector values);
	u32			anim_play			(const shared_str& anim_name, BOOL bMixIn, const CMotionDef*& md, u8& rnd, bool ignore_anm_type);
};

class player_hud
{
public: 
					player_hud			();
					~player_hud			();
	void			load				(const shared_str& model_name);
	void			load_default		(){load("actor_hud_05");};
	void			update				(Fmatrix CR$ trans);
	void			render_hud			();	
	void			render_item_ui		();
	bool			render_item_ui_query();
	u32				anim_play			(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed);
	const shared_str& section_name		() const {return m_sect_name;}

	attachable_hud_item* create_hud_item(LPCSTR hud_section, LPCSTR object_section);

	void			attach_item			(CHudItem* item);
	bool			allow_activation	(CHudItem* item);
	attachable_hud_item* attached_item	(u16 item_idx)	{return m_attached_items[item_idx];};
	void			detach_item_idx		(u16 idx);
	void			detach_item			(CHudItem* item);
	void			detach_all_items	(){m_attached_items[0]=NULL; m_attached_items[1]=NULL;};

	void			calc_transform		(u16 attach_slot_idx, Dmatrix CR$ offset, Dmatrix& result);
	void			tune				(Ivector values);
	u32				motion_length		(const MotionID& M, const CMotionDef*& md, float speed);
	u32				motion_length		(const shared_str& anim_name, const shared_str& hud_name, const shared_str& section, const CMotionDef*& md);
	void			OnMovementChanged	();
	bool			inertion_allowed	();

private:
	shared_str							m_sect_name;

	Dmatrix								m_transform[2] = { Didentity, Didentity };
	IKinematicsAnimated*				m_model[2] = { nullptr, nullptr };
	xr_vector<u16>						m_ancors = {};
	attachable_hud_item*				m_attached_items[2] = { nullptr, nullptr };
	xr_vector<attachable_hud_item*>		m_pool = {};

private:
	enum eMovementLayers
	{
		eIdle = 0,
		eWalk,
		eRun,
		eSprint,
		move_anms_end
	};
	enum eWeaponStateLayers
	{
		eRelaxed = 0,
		eArmed,
		eAim,
		eADS,
		state_anms_end
	};
	enum ePoseLayers
	{
		eStand = 0,
		eCrouch,
		pose_anms_end
	};
	movement_layer						m_movement_layers[move_anms_end][state_anms_end][pose_anms_end];
	xr_vector<xptr<script_layer>>		m_script_layers;

	void								re_sync_anim							(int part);

public:
	void								updateMovementLayerState				();
	script_layer*						playBlendAnm							(shared_str CR$ name, u8 part = 0, float speed = 1.f, float power = 1.f, bool bLooped = true, bool no_restart = false, bool full_blend = false);
	void								StopBlendAnm							(shared_str CR$ name, bool bForce = false);
};

extern player_hud* g_player_hud;
