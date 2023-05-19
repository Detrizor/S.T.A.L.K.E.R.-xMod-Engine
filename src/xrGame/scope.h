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

struct SRangeFloat
{
	float vmin, vmax, step, current;
	bool dynamic;

	void Load(LPCSTR S)
	{
		int r = sscanf(S, "%f,%f,%f", &vmin, &step, &vmax);
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
	SRangeFloat							m_Magnificaion;
	shared_str							m_Reticle;
	shared_str							m_AliveDetector;
	shared_str							m_Nighvision;
	float								m_fLenseRadius;
	CUIStatic*							m_pUIReticle;
	CBinocularsVision*					m_pVision;
	CNightVisionEffector*				m_pNight_vision;
	float								m_fZeroing;
	float								m_fDefaultZeroing;
	bool								m_fZeroingMagnificationPower;
	
	void								InitVisors								();
	void								OnZoomChange							();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	static	Fvector						lense_circle_scale;
	static	Fvector2					lense_circle_offset;

	shared_str							sight_bone_name;
	Fvector								sight_offset;

	eScopeType							Type								C$	()		{ return m_Type; }
	float								GetLenseRadius						C$	()		{ return m_fLenseRadius; }
	float								GetCurrentMagnification				C$	()		{ return m_Magnificaion.current; }
	float								Zeroing								C$	()		{ return m_fZeroing; }

	float								GetReticleScale						C$	(CWeaponHud CR$ hud);
	void								modify_holder_params				C$	(float &range, float &fov);
	bool								HasLense							C$	();

	void								ZoomChange								(int val);
	void								RenderUI								(CWeaponHud CR$ hud, Fvector2 axis_deviation);
};
