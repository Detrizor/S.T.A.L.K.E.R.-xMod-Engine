#include "stdafx.h"
#include "scope.h"
#include "player_hud.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "weaponBinocularsVision.h"
#include "Torch.h"
#include "Actor.h"
#include "weapon_hud.h"
#include "addon.h"
#include "ActorEffector.h"
#include "ai_sounds.h"
#include "Level_Bullet_Manager.h"

CUIStatic* pUILenseCircle				= NULL;
CUIStatic* pUILenseVignette				= NULL;
CUIStatic* pUILenseBlackFill			= NULL;
CUIStatic* pUILenseGlass				= NULL;

float CScope::s_magnification_eye_relief_shrink;
float CScope::s_lense_circle_scale_default;
float CScope::s_lense_circle_scale_offset_power;
SPowerDependency CScope::s_lense_circle_pos_from_axis;
SPowerDependency CScope::s_lense_circle_pos_from_zoom;
float CScope::s_lense_vignette_a;
float CScope::s_lense_vignette_b;

ref_sound CScope::m_zoom_sound;
ref_sound CScope::m_zeroing_sound;

void createStatic(CUIStatic*& dest, LPCSTR texture, float size, EAlignment al = aCenter)
{
	dest								= xr_new<CUIStatic>();
	dest->InitTextureEx					(texture);
	dest->SetTextureRect				({ 0.f, 0.f, size, size });
	dest->SetPosSize					(2, 100.f, sScreenHeight);
	dest->SetPosSize					(3, 100.f, sScreenHeight);
	dest->SetAlignment					(al);
	dest->SetAnchor						(al);
	dest->SetStretchTexture				(true);
}

void CScope::loadStaticVariables()
{
	s_magnification_eye_relief_shrink	= pSettings->r_float("scope_manager", "magnification_eye_relief_shrink");
	s_lense_circle_scale_default		= pSettings->r_float("scope_manager", "lense_circle_scale_default");
	s_lense_circle_scale_offset_power	= pSettings->r_float("scope_manager", "lense_circle_scale_offset_power");
	s_lense_circle_pos_from_axis.Load	("scope_manager", "lense_circle_pos_from_axis");
	s_lense_circle_pos_from_zoom.Load	("scope_manager", "lense_circle_pos_from_zoom");

	float lense_vignette_offset_max		= pSettings->r_float("scope_manager", "lense_vignette_offset_max");
	float lense_vignette_scale_max		= pSettings->r_float("scope_manager", "lense_vignette_scale_max");
	CScope::s_lense_vignette_a			= lense_vignette_scale_max * (1.f - lense_vignette_offset_max) / (1.f - lense_vignette_scale_max);
	CScope::s_lense_vignette_b			= CScope::s_lense_vignette_a - lense_vignette_offset_max;
	
	float lense_circle_size				= pSettings->r_float("scope_manager", "lense_circle_size");
	float lense_vignette_size			= pSettings->r_float("scope_manager", "lense_vignette_size");
	float lense_black_fill_size			= pSettings->r_float("scope_manager", "lense_black_fill_size");
	float lense_glass_size				= pSettings->r_float("scope_manager", "lense_glass_size");

	createStatic						(pUILenseCircle, "wpn\\lense\\circle", lense_circle_size);
	createStatic						(pUILenseVignette, "wpn\\lense\\vignette", lense_vignette_size);
	createStatic						(pUILenseBlackFill, "wpn\\lense\\black_fill", lense_black_fill_size, aLeftTop);
	createStatic						(pUILenseGlass, "wpn\\lense\\glass", lense_glass_size);
	
	LPCSTR zoom_sound					= pSettings->r_string("scope_manager", "zoom_sound");
	LPCSTR zeroing_sound				= pSettings->r_string("scope_manager", "zeroing_sound");
	m_zoom_sound.create					(zoom_sound, st_Effect, SOUND_TYPE_NO_SOUND);
	m_zeroing_sound.create				(zeroing_sound, st_Effect, SOUND_TYPE_NO_SOUND);
}

void CScope::cleanStaticVariables()
{
	xr_delete							(pUILenseCircle);
	xr_delete							(pUILenseVignette);
	xr_delete							(pUILenseBlackFill);
	xr_delete							(pUILenseGlass);

	m_zoom_sound.destroy				();
	m_zeroing_sound.destroy				();
}

void CScope::ZoomChange(int val)
{
	if (val > 0 && m_Magnificaion.current < m_Magnificaion.vmax ||
		val < 0 && m_Magnificaion.current > m_Magnificaion.vmin)
	{
		m_Magnificaion.Shift			(val);
		ref_sound						snd;
		snd.clone						(m_zoom_sound, st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(O.Cast<CObject*>(), sm_2D);
	}
}

void CScope::ZeroingChange(int val)
{
	if (val > 0 && m_Zeroing.current < m_Zeroing.vmax ||
		val < 0 && m_Zeroing.current > m_Zeroing.vmin)
	{
		m_Zeroing.Shift					(val);
		ref_sound						snd;
		snd.clone						(m_zeroing_sound, st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(O.Cast<CObject*>(), sm_2D);
	}
}

CScope::CScope(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_Type((eScopeType)pSettings->r_u8(section, "type")),
m_sight_position(pSettings->r_fvector3(section, "sight_position"))
{
	m_Zeroing.Load						(pSettings->r_string(section, "zeroing"));
	switch (m_Type)
	{
		case eOptics:
			m_Magnificaion.Load			(pSettings->r_string(section, "magnification"));
			m_lense_radius				= pSettings->r_float(section, "lense_radius");
			m_objective_offset			= pSettings->r_fvector3(section, "objective_offset");
			m_eye_relief				= pSettings->r_float(section, "eye_relief");
			m_Reticle					= pSettings->r_string(section, "reticle");
			m_reticle_size				= pSettings->r_float(section, "reticle_size");
			m_is_FFP					= !!pSettings->r_bool(section, "first_focal_plane");
			m_AliveDetector				= pSettings->r_string(section, "alive_detector");
			m_Nighvision				= pSettings->r_string(section, "nightvision");
			init_visors					();
			break;
		case eCollimator:
			m_Magnificaion.Load			(pSettings->r_string(section, "reticle_scale"));
			if (m_Magnificaion.dynamic)
				m_Magnificaion.current	= (m_Magnificaion.vmin + m_Magnificaion.vmax) / 2.f;
			break;
	}
}

CScope::~CScope()
{
	xr_delete							(m_pUIReticle);
	xr_delete							(m_pVision);
	xr_delete							(m_pNight_vision);
}

float CScope::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eInstallUpgrade:
		{
			bool result					= false;
			LPCSTR section				= (LPCSTR)data;
			bool test					= !!param;
			if (Type() == eOptics)
			{
				result					|= CInventoryItem::process_if_exists(section, "reticle", m_Reticle, test);
				result					|= CInventoryItem::process_if_exists(section, "alive_detector", m_AliveDetector, test);
				result					|= CInventoryItem::process_if_exists(section, "nightvision", m_Nighvision, test);
				if (result)
					init_visors			();
			}
			return						(result) ? 1.f : flt_max;
		}
		case eSyncData:
		{
			auto se_obj					= (CSE_ALifeDynamicObject*)data;
			auto m						= se_obj->getModule<CSE_ALifeModuleScope>(!!param);
			if (param)
			{
				m->m_magnification		= m_Magnificaion.current;
				m->m_zeroing			= m_Zeroing.current;
				m->m_selection			= m_selection;
			}
			else if (m)
			{
				m_Magnificaion.current	= m->m_magnification;
				m_Zeroing.current		= m->m_zeroing;
				m_selection				= m->m_selection;
			}
		}
	}

	return								CModule::aboba(type, data, param);
}

void CScope::init_visors()
{
	xr_delete							(m_pUIReticle);
	if (m_Reticle.size())
	{
		createStatic					(m_pUIReticle, *shared_str().printf("wpn\\reticle\\%s", *m_Reticle), m_reticle_size);
		m_pUIReticle->EnableHeading		(true);
	}

	xr_delete							(m_pVision);
	if (m_AliveDetector.size())
		m_pVision						= xr_new<CBinocularsVision>(m_AliveDetector);
	
	if (m_pNight_vision)
	{
		m_pNight_vision->Stop			(100000.f, false);
		xr_delete						(m_pNight_vision);
	}
	if (m_Nighvision.size())
		m_pNight_vision					= xr_new<CNightVisionEffector>(m_Nighvision);
}

void CScope::modify_holder_params(float &range, float &fov) const
{
	range								*= m_Magnificaion.vmax;
}

bool CScope::isPiP() const
{
	return								Type() == eOptics && !fIsZero(m_lense_radius);
}

void CScope::RenderUI()
{
	bool svp							= Device.m_SecondViewport.isRendering();
	if (svp)
		Device.m_SecondViewport.toggleRendering();

	if (m_pNight_vision && !m_pNight_vision->IsActive())
		m_pNight_vision->Start			(m_Nighvision, Actor(), false);

	if (m_pVision)
	{
		m_pVision->Update				();
		m_pVision->Draw					();
	}

	float magnification					= 0.f;
	if (m_Magnificaion.dynamic)
		magnification					= (m_Magnificaion.current - m_Magnificaion.vmin) / (m_Magnificaion.vmax - m_Magnificaion.vmin);
	float cur_eye_relief				= m_eye_relief * (1.f - magnification * s_magnification_eye_relief_shrink);
	float offset						= m_camera_lense_distance / cur_eye_relief;
	float scale							= s_lense_circle_scale_default * pow(offset, s_lense_circle_scale_offset_power);
	
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	float lense_scale					= hud_params.w;

	Fvector2 pos						= {
		s_lense_circle_pos_from_axis.Calc(abs(m_cam_pos_ort.x)),
		s_lense_circle_pos_from_axis.Calc(abs(m_cam_pos_ort.y))
	};
	if (m_cam_pos_ort.x < 0.f)
		pos.x							*= -1.f;
	if (m_cam_pos_ort.y > 0.f)
		pos.y							*= -1.f;
	pos.mul								(s_lense_circle_pos_from_zoom.Calc(m_Magnificaion.current));

	pUILenseCircle->SetScale			(lense_scale * scale);
	pUILenseCircle->SetWndPos			(pos);

	Frect crect							= pUILenseCircle->GetWndRect();
	if (crect.left >= UI_BASE_WIDTH || crect.right <= 0.f || crect.top >= UI_BASE_HEIGHT || crect.bottom <= 0.f)
	{
		pUILenseBlackFill->SetWndRect	({ 0.f, 0.f, UI_BASE_WIDTH, UI_BASE_HEIGHT });
		pUILenseBlackFill->Draw			();
	}
	else
	{
		if (crect.top > 0.f)
		{
			pUILenseBlackFill->SetWndRect({ 0.f, 0.f, crect.right, crect.top + 1.f });
			pUILenseBlackFill->Draw		();
		}
		if (crect.right < UI_BASE_WIDTH)
		{
			pUILenseBlackFill->SetWndRect({ crect.right - 1.f, 0.f, UI_BASE_WIDTH, crect.bottom });
			pUILenseBlackFill->Draw		();
		}
		if (crect.bottom < UI_BASE_HEIGHT)
		{
			pUILenseBlackFill->SetWndRect({ crect.left, crect.bottom - 1.f, UI_BASE_WIDTH, UI_BASE_HEIGHT });
			pUILenseBlackFill->Draw		();
		}
		if (crect.left > 0.f)
		{
			pUILenseBlackFill->SetWndRect({ 0.f, crect.top, crect.left + 1.f, UI_BASE_HEIGHT });
			pUILenseBlackFill->Draw		();
		}
		
		pUILenseCircle->Draw			();

		if (offset > 1.f)
		{
			scale						= max(s_lense_vignette_a / (offset + s_lense_vignette_b), 1.f);
			pUILenseVignette->SetScale	(lense_scale * scale);
			pUILenseVignette->Draw		();
		}
	}
	
	if (m_pUIReticle)
	{
		float magnification				= (m_is_FFP) ? (m_Magnificaion.current / m_Magnificaion.vmin) : 1.f;
		Fvector2 derivation				= { hud_params.x * UI_BASE_WIDTH, hud_params.y * UI_BASE_HEIGHT };

		m_pUIReticle->SetScale			(lense_scale * magnification);
		m_pUIReticle->SetWndPos			(derivation);
		m_pUIReticle->SetHeading		(d_hpb.z);
		m_pUIReticle->Draw				();
	}
	

	if (!isPiP())
	{
		pUILenseGlass->Draw				();
		pUILenseCircle->SetScale		(1.f);
		pUILenseCircle->SetWndPos		(Fvector2().set(0.f, 0.f));
		pUILenseCircle->Draw			();
	}

	if (svp)
		Device.m_SecondViewport.toggleRendering();
}

void CScope::updateCameraLenseOffset()
{
	static Fmatrix						trans;
	static Fmatrix						itrans;
	static Fvector						camera_lense_offset;

	auto addon							= cast<CAddon*>();
	trans								= (addon) ? addon->getHudTransform() : O.Cast<CHudItem*>()->HudItemData()->m_transform;
	trans.applyOffset					(m_sight_position, vZero);

	camera_lense_offset					= trans.c;
	camera_lense_offset.sub				(Actor()->Cameras().Position());
	m_camera_lense_distance				= camera_lense_offset.magnitude();

	itrans.invert						(trans);
	itrans.transform_tiny				(m_cam_pos_ort, Actor()->Cameras().Position());
}

float CScope::getLenseFovTan()
{
	return								m_lense_radius / m_camera_lense_distance;
}

float CScope::GetReticleScale() const
{
	return (Type() == eCollimator) ? m_Magnificaion.current : 1.f;
}
