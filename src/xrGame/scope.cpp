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

float MScope::s_eye_relief_magnification_shrink;
float MScope::s_shadow_pos_d_axis_factor;

float MScope::s_shadow_scale_default;
float MScope::s_shadow_scale_offset_power;

float MScope::s_shadow_far_scale_default;
float MScope::s_shadow_far_scale_offset_power;

shared_str MScope::s_zoom_sound;
shared_str MScope::s_zeroing_sound;
shared_str MScope::s_reticle_sound;

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

void MScope::loadStaticData()
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
	s_reticle_sound						= pSettings->r_string("scope_manager", "reticle_sound");
}

void MScope::cleanStaticVariables()
{
	xr_delete							(static_scope_shadow_far);
	xr_delete							(static_scope_shadow);
	xr_delete							(static_black_fill);
	xr_delete							(static_glass);
}

void MScope::ZoomChange(int val)
{
	if (val > 0 && m_Magnificaion.current < m_Magnificaion.vmax ||
		val < 0 && m_Magnificaion.current > m_Magnificaion.vmin)
	{
		m_Magnificaion.Shift			(val);
		ref_sound						snd;
		snd.create						(s_zoom_sound.c_str(), st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(&O, sm_2D);
	}
}

void MScope::ZeroingChange(int val)
{
	if (val > 0 && m_Zeroing.current < m_Zeroing.vmax ||
		val < 0 && m_Zeroing.current > m_Zeroing.vmin)
	{
		m_Zeroing.Shift					(val);
		ref_sound						snd;
		snd.create						(s_zeroing_sound.c_str(), st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(&O, sm_2D);
	}
}

bool MScope::reticleChange(int val)
{
	if (m_reticles_count > 1)
	{
		int new_reticle					= m_current_reticle + val;
		if (new_reticle >= m_reticles_count)
			new_reticle					= 0;
		else if (new_reticle < 0)
			new_reticle					= m_reticles_count - 1;
		m_current_reticle				= new_reticle;
		init_marks						();
		
		ref_sound						snd;
		snd.create						(s_reticle_sound.c_str(), st_Effect, SOUND_TYPE_NO_SOUND);
		snd.play						(&O, sm_2D);

		return							true;
	}
	return								false;
}

MScope::MScope(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_Type((eScopeType)pSettings->r_u8(section, "type")),
m_ads_speed_factor(pSettings->r_float(section, "ads_speed_factor"))
{
	m_sight_position					= pSettings->r_dvector3(section, (m_Type == eIS) ? "align_rear" : "sight_position");
	m_Zeroing.Load						(pSettings->r_string(section, "zeroing"));
	switch (m_Type)
	{
		case eOptics:
			m_Magnificaion.Load			(pSettings->r_string(section, "magnification"));
			m_objective_diameter		= pSettings->r_float(section, "objective_diameter") * 0.001f;
			m_eye_relief				= pSettings->r_float(section, "eye_relief") * 0.001f;

			m_reticle					= pSettings->r_string(section, "reticle");
			m_AliveDetector				= pSettings->r_string(section, "alive_detector");
			m_Nighvision				= pSettings->r_string(section, "nightvision");

			m_lense_radius				= pSettings->r_float(section, "lense_radius");
			m_objective_offset			= pSettings->r_fvector3(section, "objective_offset");

			if (pSettings->line_exist(section, "backup_sight"))
				m_backup_sight.construct(obj, pSettings->r_string(section, "backup_sight"));

			break;

		case eCollimator:
			m_reticles_count			= pSettings->r_u8(section, "reticles_count");
			m_Magnificaion.Load			(pSettings->r_string(section, "reticle_scale"));
			if (m_Magnificaion.dynamic)
				m_Magnificaion.current	= (m_Magnificaion.vmin + m_Magnificaion.vmax) / 2.f;
			break;
	}
}

MScope::~MScope()
{
	xr_delete							(m_pUIReticle);
	xr_delete							(m_pVision);
	xr_delete							(m_pNight_vision);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MScope::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto m								= se_obj->getModule<CSE_ALifeModuleScope>(save);
	if (save)
	{
		m->m_magnification				= m_Magnificaion.current;
		m->m_zeroing					= m_Zeroing.current;
		m->m_selection					= static_cast<s8>(m_selection);
		m->m_current_reticle			= m_current_reticle;
	}
	else if (m)
	{
		m_Magnificaion.current			= m->m_magnification;
		m_Zeroing.current				= m->m_zeroing;
		m_selection						= static_cast<ScopeSelectionType>(m->m_selection);
		m_current_reticle				= m->m_current_reticle;
	}
	if (!save)
		init_marks						();
}

void MScope::sNetRelcase(CObject* obj)
{
	if (m_pVision)
		m_pVision->remove_links			(obj);
}

bool MScope::sInstallUpgrade(LPCSTR section, bool test)
{
	bool result							= false;
	if (Type() == eOptics)
	{
		result							|= CInventoryItem::process_if_exists(section, "reticle", m_reticle, test);
		result							|= CInventoryItem::process_if_exists(section, "alive_detector", m_AliveDetector, test);
		result							|= CInventoryItem::process_if_exists(section, "nightvision", m_Nighvision, test);
		if (result)
			init_visors					();
	}
	return								result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MScope::init_visors()
{
	xr_delete							(m_pUIReticle);
	if (m_reticle.size())
	{
		CUIXml							xml;
		xml.Load						(CONFIG_PATH, UI_PATH, "scopes.xml");
		m_reticles_count				= xml.GetNodesNum("w", 0, m_reticle.c_str());
		m_is_FFP						= !!xml.ReadAttribInt(m_reticle.c_str(), 0, "ffp", 0);

		m_pUIReticle					= xr_new<CUIStatic>();
		CUIXmlInit::InitStatic			(xml, m_reticle.c_str(), m_current_reticle, m_pUIReticle);
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

void MScope::init_marks()
{
	switch (m_Type)
	{
	case eOptics:
		init_visors						();
		break;
	case eCollimator:
		if (m_reticles_count > 1)
			for (int i = 0; i < m_reticles_count; i++)
				O.UpdateBoneVisibility	(shared_str().printf("mark_%d", i), i == m_current_reticle);
		break;
	}
}

void MScope::modify_holder_params(float &range, float &fov) const
{
	range								*= m_Magnificaion.vmax;
}

bool MScope::isPiP() const
{
	return								(Type() == eOptics && !fIsZero(m_lense_radius));
}

float MScope::update_scope_shadow()
{
	float magnification					= 0.f;
	if (m_Magnificaion.dynamic)
		magnification					= (m_Magnificaion.current - m_Magnificaion.vmin) / (m_Magnificaion.vmax - m_Magnificaion.vmin);
	float cur_eye_relief				= m_eye_relief * (1.f - magnification * s_eye_relief_magnification_shrink);
	float offset						= (fMore(cur_eye_relief, 0.f)) ? m_camera_lense_distance / cur_eye_relief : 1.f;
	
	float lense_fov_tan					= m_lense_radius / m_camera_lense_distance;
	m_lense_scale						= lense_fov_tan / Device.aimFOVTan;
	float scale							= m_lense_scale;
	if (fMore(offset, 1.f))
	{
		m_scope_shadow					= static_scope_shadow_far;
		scale							*= s_shadow_far_scale_default * pow(offset, s_shadow_far_scale_offset_power);
	}
	else
	{
		m_scope_shadow					= static_scope_shadow;
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

	m_scope_shadow->SetScale			(scale);
	m_scope_shadow->SetWndPos			(pos);

	return								scale;
}

void MScope::RenderUI()
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

	Frect crect = m_scope_shadow->GetWndRect();
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
		
		m_scope_shadow->Draw			();
	}
	
	if (m_pUIReticle)
	{
		auto& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
		Fvector2 derivation				= { hud_params.x * UI_BASE_HEIGHT, hud_params.y * UI_BASE_HEIGHT };
		
		float magnification				= (m_is_FFP) ? (m_Magnificaion.current / m_Magnificaion.vmin) : 1.f;
		m_pUIReticle->SetScale			(m_lense_scale * magnification);
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

void MScope::updateCameraLenseOffset()
{
	auto addon							= O.getModule<MAddon>();
	Dmatrix trans						= (addon) ? addon->getHudTransform() : O.scast<CHudItem*>()->HudItemData()->m_transform;
	trans.translate_mul					(m_sight_position);

	m_camera_lense_offset				= static_cast<Fvector>(trans.c);
	m_camera_lense_offset.sub			(Actor()->Cameras().Position());
	m_camera_lense_distance				= m_camera_lense_offset.magnitude();

	Dmatrix								itrans;
	itrans.invert						(trans);
	itrans.transform_tiny				(m_cam_pos_d_sight_axis, static_cast<Dvector>(Actor()->Cameras().Position()));
}

void MScope::updateSVP(Dmatrix CR$ transform)
{
	if (Type() == MScope::eIS)
		return;

	static Fmatrix						cam_trans;
	static Fvector						cam_hpb;
	static Dvector						self_hpb;
	static Fvector						d_hpb;

	Actor()->Cameras().camera_Matrix	(cam_trans);
	cam_trans.getHPB					(cam_hpb);
	transform.getHPB					(self_hpb);
	d_hpb								= cam_hpb;
	d_hpb.sub							(static_cast<Fvector>(self_hpb));

	float fov_tan						= Device.aimFOVTan;
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	if (Type() == eOptics)
	{
		fov_tan							/= m_Magnificaion.current;
		float shadow_scale				= update_scope_shadow();
		Device.SVP.setViewFOV			(2.f * atanf(fov_tan * min(m_lense_scale, shadow_scale)));
		Device.SVP.setLenseFOV			(2.f * atanf(Device.aimFOVTan * m_lense_scale));
		Device.SVP.setLenseDir			(m_camera_lense_offset);

		Device.SVP.setFOV				(rad2degHalf(atanf(fov_tan)));
		Device.SVP.setZoom				(m_Magnificaion.current);
		
		Fvector pos						= m_objective_offset;
		cam_trans.transform_tiny		(pos);
		Device.SVP.setPosition			(pos);
	}
	else
		hud_params.w					= m_Magnificaion.current;

	float distance						= Zeroing();
	float x_derivation					= distance * tanf(d_hpb.x);
	float y_derivation					= distance * tanf(d_hpb.y);

	float h								= 2.f * fov_tan * distance;
	hud_params.x						= x_derivation / h;
	hud_params.y						= y_derivation / h;
	hud_params.z						= d_hpb.z;
}
