#pragma once

#include "module.h"

class CUIStatic;
class CBinocularsVision;
class CNightVisionEffector;
class CWeaponHud;
class CAddon;
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

class CScope : public CModule
{
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

public:
										CScope									(CGameObject* obj, shared_str CR$ section);
										~CScope									();

private:
	const eScopeType					m_Type;
	const float							m_ads_speed_factor;
	
	CUIStatic*							m_pUIReticle							= NULL;
	CBinocularsVision*					m_pVision								= NULL;
	CNightVisionEffector*				m_pNight_vision							= NULL;
	float								m_camera_lense_distance					= 0.f;
	Dvector								m_hud_offset[2]							= { dZero, dZero };
	s8									m_selection								= -1;
	Dvector								m_cam_pos_d_sight_axis					= dZero;

	Dvector								m_sight_offset[2];
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
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	static void							loadStaticVariables						();
	static void							cleanStaticVariables					();

	void								ZoomChange								(int val);
	void								ZeroingChange							(int val);
	void								setHudOffset							(Dvector CP$ v)		{ m_hud_offset[0] = v[0]; m_hud_offset[1] = v[1]; }
	void								setMagnificaiton						(float val)			{ m_Magnificaion.current = val; }
	void								setZeroing								(u16 val)			{ m_Zeroing.current = val; }
	void								setSelection							(s8 val)			{ m_selection = val; }

	void								RenderUI								();
	void								updateCameraLenseOffset					();
	float								getLenseFovTan							();

	eScopeType							Type								C$	()		{ return m_Type; }
	float								GetCurrentMagnification				C$	()		{ return m_Magnificaion.current; }
	u16									Zeroing								C$	()		{ return m_Zeroing.current; }
	Dvector CR$							getObjectiveOffset					C$	()		{ return m_objective_offset; }
	Dvector CP$							getSightOffset						C$	()		{ return m_sight_offset; }
	float								getEyeRelief						C$	()		{ return m_eye_relief; }
	Dvector CP$							getHudOffset						C$	()		{ return m_hud_offset; }
	s8									getSelection						C$	()		{ return m_selection; }
	float								getAdsSpeedFactor					C$	()		{ return m_ads_speed_factor; }

	float								GetReticleScale						C$	();
	void								modify_holder_params				C$	(float &range, float &fov);
	bool								isPiP								C$	();
};
