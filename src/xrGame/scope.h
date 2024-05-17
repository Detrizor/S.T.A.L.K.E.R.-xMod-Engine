#pragma once

#include "module.h"

enum eScopeType
{
	eOptics = 1,
	eCollimator,
	eIS
};

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
										CScope									(CGameObject* obj, shared_str CR$ section);
										~CScope									();

private:
	static float						s_magnification_eye_relief_shrink;
	static float						s_lense_circle_scale_default;
	static float						s_lense_circle_scale_offset_power;
	static float						s_lense_circle_position_derivation_factor;
	static float						s_lense_vignette_a;
	static float						s_lense_vignette_b;

	static ref_sound					m_zoom_sound;
	static ref_sound					m_zeroing_sound;

	const eScopeType					m_Type;
	const Fvector						m_sight_position;
	
	CUIStatic*							m_pUIReticle							= NULL;
	CBinocularsVision*					m_pVision								= NULL;
	CNightVisionEffector*				m_pNight_vision							= NULL;
	Fvector								m_camera_lense_offset					= vZero;
	Fvector								m_hud_offset[2]							= { vZero, vZero };
	s8									m_selection								= -1;

	SRangeNum<u16>						m_Zeroing;
	SRangeNum<float>					m_Magnificaion;

	float								m_lense_radius;
	Fvector								m_objective_offset;
	float								m_eye_relief;
	shared_str							m_Reticle;
	float								m_reticle_size;
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
	void								setHudOffset							(Fvector CP$ v)		{ m_hud_offset[0] = v[0]; m_hud_offset[1] = v[1]; }
	void								setMagnificaiton						(float val)			{ m_Magnificaion.current = val; }
	void								setZeroing								(u16 val)			{ m_Zeroing.current = val; }
	void								setSelection							(s8 val)			{ m_selection = val; }

	void								RenderUI								();
	void								updateCameraLenseOffset					();
	float								getLenseFovTan							();

	eScopeType							Type								C$	()		{ return m_Type; }
	float								GetCurrentMagnification				C$	()		{ return m_Magnificaion.current; }
	u16									Zeroing								C$	()		{ return m_Zeroing.current; }
	Fvector CR$							getObjectiveOffset					C$	()		{ return m_objective_offset; }
	Fvector CR$							getSightPosition					C$	()		{ return m_sight_position; }
	float								getEyeRelief						C$	()		{ return m_eye_relief; }
	Fvector CP$							getHudOffset						C$	()		{ return m_hud_offset; }
	s8									getSelection						C$	()		{ return m_selection; }

	float								GetReticleScale						C$	();
	void								modify_holder_params				C$	(float &range, float &fov);
	bool								isPiP								C$	();
};
