#pragma once

#include "module.h"

enum eScopeType
{
	eOptics,
	eCollimator
};

class CUIStatic;
class CBinocularsVision;
class CNightVisionEffector;

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
										CScope								(CGameObject* obj);
							~CScope					();
	void					Load					(LPCSTR section);
	
protected:
			eScopeType		m_Type;
			SRangeFloat		m_Magnificaion;
			shared_str		m_Reticle;
			shared_str		m_AliveDetector;
			shared_str		m_Nighvision;
			float			m_fLenseRadius;
			CUIStatic*		m_pUIReticle;
	CBinocularsVision*		m_pVision;
	CNightVisionEffector*	m_pNight_vision;

			bool			install_upgrade_impl	(LPCSTR section, bool test);
			void			InitVisors				();
			float			ReticleCircleOffset		(int idx, const hud_item_measures& m, Fvector* const hud_offset) const;

public:
	static	Fvector			lense_circle_scale;
	static	Fvector4		lense_circle_offset[2];

			eScopeType		Type					()								const	{ return m_Type; }
			void			modify_holder_params	(float &range, float &fov)		const;
			void			ZoomChange				(int val);
			bool			HasLense				()								const;
			float			GetLenseRadius			()								const	{ return m_fLenseRadius; }
			float			GetReticleScale			()								const	{ return m_Magnificaion.current; }
			float			GetCurrentMagnification	()								const	{ return m_Magnificaion.current; }
			void			RenderUI				(const hud_item_measures& m, Fvector* const hud_offset);
};