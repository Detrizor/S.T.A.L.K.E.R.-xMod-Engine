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

CUIStatic* pUILenseCircle				= NULL;
CUIStatic* pUILenseVignette				= NULL;
CUIStatic* pUILenseBlackFill			= NULL;
CUIStatic* pUILenseGlass				= NULL;

void createStatic(CUIStatic*& dest, LPCSTR texture, float mult = 1.f, EAlignment al = aCenter)
{
	dest								= xr_new<CUIStatic>();
	dest->InitTextureEx					(texture);
	dest->SetTextureRect				({ 0.f, 0.f, mult * 1024.f, mult * 1024.f });
	dest->SetPosSize					(2, 1.f, sScreenHeight);
	dest->SetPosSize					(3, 1.f, sScreenHeight);
	dest->SetAlignment					(al);
	dest->SetAnchor						(al);
	dest->SetStretchTexture				(true);
}

CScope::CScope(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_addon(obj->Cast<CAddon*>()),
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

float CScope::aboba o$(EEventTypes type, void* data, int param)
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
	}

	return								CModule::aboba(type, data, param);
}

void CScope::init_visors()
{
	xr_delete							(m_pUIReticle);
	if (m_Reticle.size())
		createStatic					(m_pUIReticle, shared_str().printf("wpn\\reticle\\%s", *m_Reticle).c_str());

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

void CScope::modify_holder_params C$(float &range, float &fov)
{
	if (Type() == eOptics)
	{
		range							*= m_Magnificaion.vmax;
		fov								*= pow(m_Magnificaion.vmax, .25f);
	}
}

bool CScope::isPiP() const
{
	return								Type() == eOptics && !fIsZero(m_lense_radius);
}

extern bool force_draw_ui_on_svp;
void CScope::RenderUI()
{
	force_draw_ui_on_svp				= true;

	if (m_pNight_vision && !m_pNight_vision->IsActive())
		m_pNight_vision->Start			(m_Nighvision, Actor(), false);

	if (m_pVision)
	{
		m_pVision->Update				();
		m_pVision->Draw					();
	}
	
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	Fvector2 derivation					= { hud_params.x * UI_BASE_WIDTH, -hud_params.y * UI_BASE_HEIGHT };

	float w2							= UI_BASE_WIDTH * .5f;
	float h2							= UI_BASE_HEIGHT * .5f;
	if (derivation.x > -w2 && derivation.x < w2 && derivation.y > -h2 && derivation.y < h2)
	{
		float magnification				= 0.f;
		if (m_Magnificaion.dynamic)
			magnification				= (m_Magnificaion.current - m_Magnificaion.vmin) / (m_Magnificaion.vmax - m_Magnificaion.vmin);
		float cur_eye_relief			= m_eye_relief * (1.f - magnification * s_magnification_eye_relief_shrink);
		float offset					= m_camera_lense_offset.magnitude() / cur_eye_relief;

		float scale						= s_lense_circle_scale_default * pow(min(offset, 1.f), s_lense_circle_scale_offset_power);
		Fvector2 pos					= derivation;
		pos.mul							(s_lense_circle_position_derivation_factor);
		pUILenseCircle->SetScale		(scale);
		pUILenseCircle->SetWndPos		(pos);
		pUILenseCircle->Draw			();

		if (offset > 1.f)
		{
			scale						= s_lense_vignette_a / (offset + s_lense_vignette_b);
			pUILenseVignette->SetScale	(max(scale, 1.f));
			pUILenseVignette->Draw		();
		}

		Frect crect						= pUILenseCircle->GetWndRect();
		pUILenseBlackFill->SetWndRect	(Frect().set(0.f, 0.f, crect.right, crect.top));
		pUILenseBlackFill->Draw			();
		pUILenseBlackFill->SetWndRect	(Frect().set(crect.right, 0.f, UI_BASE_WIDTH, crect.bottom));
		pUILenseBlackFill->Draw			();
		pUILenseBlackFill->SetWndRect	(Frect().set(crect.left, crect.bottom, UI_BASE_WIDTH, UI_BASE_HEIGHT));
		pUILenseBlackFill->Draw			();
		pUILenseBlackFill->SetWndRect	(Frect().set(0.f, crect.top, crect.left, UI_BASE_HEIGHT));
		pUILenseBlackFill->Draw			();
	}
	else
	{
		pUILenseBlackFill->SetWndRect	({ 0.f, 0.f, UI_BASE_WIDTH, UI_BASE_HEIGHT });
		pUILenseBlackFill->Draw			();
	}
	
	if (m_pUIReticle)
	{
		m_pUIReticle->SetWndPos			(derivation);
		m_pUIReticle->Draw				();
	}
	

	if (!isPiP())
	{
		pUILenseGlass->Draw				();
		pUILenseCircle->SetScale		(1.f);
		pUILenseCircle->SetWndPos		(Fvector2().set(0.f, 0.f));
		pUILenseCircle->Draw			();
	}

	force_draw_ui_on_svp				= false;
}

void CScope::updateCameraLenseOffset()
{
	Fmatrix CR$ transform				= (m_addon) ? m_addon->getHudTransform() : O.Cast<CHudItem*>()->HudItemData()->m_transform;
	transform.transform_tiny			(m_camera_lense_offset, m_sight_position);
	m_camera_lense_offset.sub			(Actor()->Cameras().Position());
}

float CScope::getLenseFovTan()
{
	return								m_lense_radius / m_camera_lense_offset.magnitude();
}

float CScope::GetReticleScale() const
{
	return (Type() == eCollimator) ? m_Magnificaion.current : 1.f;
}
