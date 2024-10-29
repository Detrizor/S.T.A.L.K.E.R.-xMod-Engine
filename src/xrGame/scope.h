#pragma once

#include "module.h"

class CUIStatic;
class CBinocularsVision;
class CNightVisionEffector;
class CWeaponHud;
class MAddon;
struct ref_sound;

struct hud_item_measures;

template<typename T>
struct SRangeNum
{
	T vmin, vmax, step, current;
	bool dynamic;

	void Load(LPCSTR S)
	{
		float _min, _step, _max;
		int r = sscanf(S, "%f,%f,%f", &_min, &_step, &_max);
		vmin = (T)_min; step = (T)_step; vmax = (T)_max;
		dynamic = (r == 3);
		current = vmin;
	}
	void Shift(int steps)
	{
		if (dynamic)
		{
			current += step * steps;
			clamp(current, vmin, vmax);
		}
	}
};

class MScope : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mScope; }

public:
	enum eScopeType
	{
		eOptics = 1,
		eCollimator,
		eIS
	};

private:
	static float						s_eye_relief_magnification_shrink;
	static float						s_shadow_pos_d_axis_factor;

	static float						s_shadow_scale_default;
	static float						s_shadow_scale_offset_power;

	static float						s_shadow_far_scale_default;
	static float						s_shadow_far_scale_offset_power;

	static shared_str					s_zoom_sound;
	static shared_str					s_zeroing_sound;
	static shared_str					s_reticle_sound;

public:
										MScope									(CGameObject* obj, shared_str CR$ section);
										~MScope									();
	
private:
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);
	void								sNetRelcase							O$	(CObject* obj);
	bool								sInstallUpgrade						O$	(LPCSTR section, bool test);

private:
	const eScopeType					m_Type;
	const float							m_ads_speed_factor;
	
	CUIStatic*							m_pUIReticle							= nullptr;
	CBinocularsVision*					m_pVision								= nullptr;
	CNightVisionEffector*				m_pNight_vision							= nullptr;
	float								m_camera_lense_distance					= 0.f;
	Dvector								m_hud_offset[2]							= { dZero, dZero };
	s8									m_selection								= -1;
	Dvector								m_cam_pos_d_sight_axis					= dZero;
	xptr<MScope>						m_backup_sight							= nullptr;
	u8									m_current_reticle						= 0;

	u8									m_reticles_count;
	Dvector								m_sight_position;
	SRangeNum<u16>						m_Zeroing;
	SRangeNum<float>					m_Magnificaion;

	float								m_lense_radius;
	Dvector								m_objective_offset;
	float								m_objective_diameter;
	float								m_eye_relief;
	shared_str							m_Reticle;
	float								m_reticle_texture_size;
	bool								m_is_FFP;
	shared_str							m_AliveDetector;
	shared_str							m_Nighvision;
	
	void								init_visors								();
	void								init_marks								();

public:
	static void							loadStaticData							();
	static void							cleanStaticVariables					();

	void								ZoomChange								(int val);
	void								ZeroingChange							(int val);
	bool								reticleChange							(int val);
	void								setHudOffset							(Dvector CR$ pos, Dvector CR$ rot)		{ m_hud_offset[0] = pos; m_hud_offset[1] = rot; }
	void								setMagnificaiton						(float val)								{ m_Magnificaion.current = val; }
	void								setZeroing								(u16 val)								{ m_Zeroing.current = val; }
	void								setSelection							(s8 val)								{ m_selection = val; }

	void								RenderUI								();
	void								updateCameraLenseOffset					();
	float								getLenseFovTan							();

	eScopeType							Type								C$	()		{ return m_Type; }
	float								GetCurrentMagnification				C$	()		{ return m_Magnificaion.current; }
	u16									Zeroing								C$	()		{ return m_Zeroing.current; }
	Dvector CR$							getObjectiveOffset					C$	()		{ return m_objective_offset; }
	Dvector CR$							getSightPosition					C$	()		{ return m_sight_position; }
	float								getEyeRelief						C$	()		{ return m_eye_relief; }
	Dvector CP$							getHudOffset						C$	()		{ return m_hud_offset; }
	s8									getSelection						C$	()		{ return m_selection; }
	float								getAdsSpeedFactor					C$	()		{ return m_ads_speed_factor; }
	MScope*								getBackupSight						C$	()		{ return (m_backup_sight) ? m_backup_sight.get() : nullptr; }

	float								GetReticleScale						C$	();
	void								modify_holder_params				C$	(float &range, float &fov);
	bool								isPiP								C$	();
};
