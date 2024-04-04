#pragma once

#include "module.h"

enum eScopeType
{
	eOptics = 1,
	eCollimator
};

class CUIStatic;
class CBinocularsVision;
class CNightVisionEffector;
class CWeaponHud;

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
	eScopeType							m_Type;
	SRangeNum<float>					m_Magnificaion;
	shared_str							m_Reticle;
	shared_str							m_AliveDetector;
	shared_str							m_Nighvision;
	float								m_fLenseRadius;
	CUIStatic*							m_pUIReticle;
	CBinocularsVision*					m_pVision;
	CNightVisionEffector*				m_pNight_vision;
	SRangeNum<u16>						m_Zeroing;
	Fvector								m_outer_lense_offset = vZero;
	
	void								InitVisors								();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	static	Fvector						s_lense_circle_scale;
	static	Fvector2					s_lense_circle_offset;

	eScopeType							Type								C$	()		{ return m_Type; }
	float								GetLenseRadius						C$	()		{ return m_fLenseRadius; }
	float								GetCurrentMagnification				C$	()		{ return m_Magnificaion.current; }
	u16									Zeroing								C$	()		{ return m_Zeroing.current; }
	Fvector CR$							getOuterLenseOffset					C$	()		{ return m_outer_lense_offset; }

	float								GetReticleScale						C$	(CWeaponHud CR$ hud);
	void								modify_holder_params				C$	(float &range, float &fov);
	bool								HasLense							C$	();

	void								ZoomChange								(int val)		{ m_Magnificaion.Shift(val); }
	void								ZeroingChange							(int val)		{ m_Zeroing.Shift(val); }

	void								RenderUI								(CWeaponHud CR$ hud);
};
