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

CUIStatic* static_scope_shadow_far		= nullptr;
CUIStatic* static_scope_shadow			= nullptr;
CUIStatic* static_black_fill			= nullptr;
CUIStatic* static_glass					= nullptr;

float CScope::s_eye_relief_magnification_shrink;
float CScope::s_shadow_pos_d_axis_factor;

float CScope::s_shadow_scale_default;
float CScope::s_shadow_scale_offset_power;

float CScope::s_shadow_far_scale_default;
float CScope::s_shadow_far_scale_offset_power;

shared_str CScope::s_zoom_sound;
shared_str CScope::s_zeroing_sound;

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
	LPCSTR shadow_far_texture			= pSettings->r_string("scope_manager", "shadow_far_texture");
	float shadow_far_texture_size		= pSettings->r_float("scope_manager", "shadow_far_texture_size");
	createStatic						(static_scope_shadow_far, shadow_far_texture, shadow_far_texture_size);
	
	LPCSTR shadow_texture				= pSettings->r_string("scope_manager", "shadow_texture");
	float shadow_texture_size			= pSettings->r_float("scope_manager", "shadow_texture_size");
	createStatic						(static_scope_shadow, shadow_texture, shadow_texture_size);
	
	LPCSTR black_fill_texture			= pSettings->r_string("scope_manager", "black_fill_texture");
	float black_fill_texture_size		= pSettings->r_float("scope_manager", "black_fill_texture_size");
	createStatic						(static_black_fill, black_fill_texture, black_fill_texture_size, aLeftTop);
	
	LPCSTR glass_texture				= pSettings->r_string("scope_manager", "glass_texture");
	float glass_texture_size			= pSettings->r_float("scope_manager", "glass_texture_size");
	createStatic						(static_glass, glass_texture, glass_texture_size);

	s_eye_relief_magnification_shrink	= pSettings->r_float("scope_manager", "eye_relief_magnification_shrink");
	s_shadow_pos_d_axis_factor			= pSettings->r_float("scope_manager", "shadow_pos_d_axis_factor");

	s_shadow_scale_default				= pSettings->r_float("scope_manager", "shadow_scale_default");
	s_shadow_scale_offset_power			= pSettings->r_float("scope_manager", "shadow_scale_offset_power");

	s_shadow_far_scale_default			= pSettings->r_float("scope_manager", "shadow_far_scale_default");
	s_shadow_far_scale_offset_power		= pSettings->r_float("scope_manager", "shadow_far_scale_offset_power");
	
	s_zoom_sound						= pSettings->r_string("scope_manager", "zoom_sound");
	s_zeroing_sound						= pSettings->r_string("scope_manager", "zeroing_sound");
}

void CScope::cleanStaticVariables()
{
	xr_delete							(static_scope_shadow_far);
	xr_delete							(static_scope_shadow);
	xr_delete							(static_black_fill);
	xr_delete							(static_glass);
}

void CScope::ZoomChange(int val)
{
	if (val > 0 && m_Magnificaion.current < m_Magnificaion.vmax ||
		val < 0 && m_Magnificaion.current > m_Magnificaion.vmin)
	{
		m_Magnificaion.Shift			(val);
		ref_sound						snd;
		snd.create						(s_zoom_sound.c_str(), st_Effect, SOUND_TYPE_NO_SOUND);
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
		snd.create						(s_zeroing_sound.c_str(), st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(O.Cast<CObject*>(), sm_2D);
	}
}

CScope::CScope(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_Type((eScopeType)pSettings->r_u8(section, "type")),
m_ads_speed_factor(pSettings->r_float(section, "ads_speed_factor"))
{
	m_sight_offset[0]					= static_cast<Dvector>(pSettings->r_fvector3(section, "sight_position"));
	m_sight_offset[1]					= static_cast<Dvector>(pSettings->r_fvector3d2r(section, "sight_rotation"));
	m_Zeroing.Load						(pSettings->r_string(section, "zeroing"));
	switch (m_Type)
	{
		case eOptics:
			m_Magnificaion.Load			(pSettings->r_string(section, "magnification"));
			m_objective_diameter		= pSettings->r_float(section, "objective_diameter") * 0.001f;
			m_eye_relief				= pSettings->r_float(section, "eye_relief") * 0.001f;

			m_Reticle					= pSettings->r_string(section, "reticle");
			m_reticle_texture_size		= pSettings->r_float(section, "reticle_texture_size");
			m_is_FFP					= !!pSettings->r_bool(section, "first_focal_plane");
			m_AliveDetector				= pSettings->r_string(section, "alive_detector");
			m_Nighvision				= pSettings->r_string(section, "nightvision");

			m_lense_radius				= pSettings->r_float(section, "lense_radius");
			m_objective_offset			= static_cast<Dvector>(pSettings->r_fvector3(section, "objective_offset"));

			init_visors					();

			if (pSettings->line_exist(section, "backup_sight"))
				m_backup_sight			= xr_new<CScope>(obj, pSettings->r_string(section, "backup_sight"));

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
		createStatic					(m_pUIReticle, *shared_str().printf("wpn\\reticle\\%s", *m_Reticle), m_reticle_texture_size);
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
	bool svp							= Device.SVP.isRendering();
	if (svp)
		Device.SVP.setRendering			(false);

	if (m_pNight_vision && !m_pNight_vision->IsActive())
		m_pNight_vision->Start			(m_Nighvision, Actor(), false);

	if (m_pVision)
	{
		m_pVision->Update				();
		m_pVision->Draw					();
	}
	
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	float lense_scale					= hud_params.w;

	float magnification					= 0.f;
	if (m_Magnificaion.dynamic)
		magnification					= (m_Magnificaion.current - m_Magnificaion.vmin) / (m_Magnificaion.vmax - m_Magnificaion.vmin);
	float cur_eye_relief				= m_eye_relief * (1.f - magnification * s_eye_relief_magnification_shrink);
	float offset						= (fMore(cur_eye_relief, 0.f)) ? m_camera_lense_distance / cur_eye_relief : 1.f;
	
	CUIStatic* shadow					= nullptr;
	float scale							= lense_scale;
	if (fMore(offset, 1.f))
	{
		shadow							= static_scope_shadow_far;
		scale							*= s_shadow_far_scale_default * pow(offset, s_shadow_far_scale_offset_power);
	}
	else
	{
		shadow							= static_scope_shadow;
		scale							*= s_shadow_scale_default * pow(offset, s_shadow_scale_offset_power);
	}

	Fvector2 pos						= {
		-static_cast<float>(m_cam_pos_d_sight_axis.x),
		static_cast<float>(m_cam_pos_d_sight_axis.y)
	};
	pos.mul								(s_shadow_pos_d_axis_factor);
	pos.mul								(m_Magnificaion.current);

	float exit_pupil_correction			= m_objective_diameter * s_shadow_pos_d_axis_factor;
	//scale								*= 100.f + exit_pupil_correction;		--xd need more research to accurately implement

	shadow->SetScale					(scale);
	shadow->SetWndPos					(pos);
		
	Frect crect							= shadow->GetWndRect();
	if (crect.left >= UI_BASE_WIDTH || crect.right <= 0.f || crect.top >= UI_BASE_HEIGHT || crect.bottom <= 0.f)
	{
		static_black_fill->SetWndRect({ 0.f, 0.f, UI_BASE_WIDTH, UI_BASE_HEIGHT });
		static_black_fill->Draw		();
	}
	else
	{
		if (crect.top > 0.f)
		{
			static_black_fill->SetWndRect({ 0.f, 0.f, crect.right, crect.top + 1.f });
			static_black_fill->Draw	();
		}
		if (crect.right < UI_BASE_WIDTH)
		{
			static_black_fill->SetWndRect({ crect.right - 1.f, 0.f, UI_BASE_WIDTH, crect.bottom });
			static_black_fill->Draw	();
		}
		if (crect.bottom < UI_BASE_HEIGHT)
		{
			static_black_fill->SetWndRect({ crect.left, crect.bottom - 1.f, UI_BASE_WIDTH, UI_BASE_HEIGHT });
			static_black_fill->Draw	();
		}
		if (crect.left > 0.f)
		{
			static_black_fill->SetWndRect({ 0.f, crect.top, crect.left + 1.f, UI_BASE_HEIGHT });
			static_black_fill->Draw	();
		}
		
		shadow->Draw				();
	}
	
	if (m_pUIReticle)
	{
		float magnification				= (m_is_FFP) ? (m_Magnificaion.current / m_Magnificaion.vmin) : 1.f;
		Fvector2 derivation				= { hud_params.x * UI_BASE_HEIGHT, hud_params.y * UI_BASE_HEIGHT };

		m_pUIReticle->SetScale			(lense_scale * magnification);
		m_pUIReticle->SetWndPos			(derivation);
		m_pUIReticle->SetHeading		(hud_params.z);
		m_pUIReticle->Draw				();
	}

	if (!isPiP())
	{
		static_glass->Draw				();
		static_scope_shadow->SetScale	(s_shadow_scale_default);
		static_scope_shadow->SetWndPos	({ 0.f, 0.f });
		static_scope_shadow->Draw		();
	}

	if (svp)
		Device.SVP.setRendering			(true);
}

void CScope::updateCameraLenseOffset()
{
	auto addon							= cast<CAddon*>();
	Dmatrix trans						= (addon) ? addon->getHudTransform() : O.Cast<CHudItem*>()->HudItemData()->m_transform;
	trans.applyOffset					(m_sight_offset);

	Dvector camera_lense_offset			= trans.c;
	camera_lense_offset.sub				(static_cast<Dvector>(Actor()->Cameras().Position()));
	m_camera_lense_distance				= camera_lense_offset.magnitude();

	Dmatrix								itrans;
	itrans.invert						(trans);
	itrans.transform_tiny				(m_cam_pos_d_sight_axis, static_cast<Dvector>(Actor()->Cameras().Position()));
}

float CScope::getLenseFovTan()
{
	return								m_lense_radius / m_camera_lense_distance;
}

float CScope::GetReticleScale() const
{
	return (Type() == eCollimator) ? m_Magnificaion.current : 1.f;
}
