#include "stdafx.h"
#include "scope.h"
#include "player_hud.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "weaponBinocularsVision.h"
#include "Torch.h"
#include "Actor.h"
#include "weapon_hud.h"

CUIStatic* pUILenseCircle				= NULL;
CUIStatic* pUILenseBlackFill			= NULL;
CUIStatic* pUILenseGlass				= NULL;

void createStatic(CUIStatic*& dest, LPCSTR texture, float mult = 1.f, EAlignment al = aCenter)
{
	dest								= xr_new<CUIStatic>();
	dest->InitTextureEx					(texture);
	Frect								rect;
	rect.set							(0.f, 0.f, mult * 1024.f, mult * 1024.f);
	dest->SetTextureRect				(rect);
	dest->SetWndSize					(Fvector2().set(mult * UI_BASE_HEIGHT, mult * UI_BASE_HEIGHT));
	dest->SetAlignment					(al);
	dest->SetAnchor						(al);
	dest->SetStretchTexture				(true);
}

CScope::CScope(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_pUIReticle						= NULL;
	m_pVision							= NULL;
	m_pNight_vision						= NULL;

	m_Zeroing.Load						(pSettings->r_string(section, "zeroing"));
	m_Type								= (eScopeType)pSettings->r_u8(section, "type");
	switch (m_Type)
	{
		case eOptics:
			m_Magnificaion.Load			(pSettings->r_string(section, "magnification"));
			m_fLenseRadius				= pSettings->r_float(section, "lense_radius");
			m_Reticle					= pSettings->r_string(section, "reticle");
			m_AliveDetector				= pSettings->r_string(section, "alive_detector");
			m_Nighvision				= pSettings->r_string(section, "nightvision");
			InitVisors					();
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
					InitVisors			();
			}
			return						(result) ? 1.f : flt_max;
		}
	}

	return								CModule::aboba(type, data, param);
}

void CScope::InitVisors()
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

bool CScope::HasLense() const
{
	return								Type() == eOptics && !fIsZero(m_fLenseRadius);
}

void CScope::RenderUI(CWeaponHud CR$ hud)
{
	if (m_pNight_vision && !m_pNight_vision->IsActive())
		m_pNight_vision->Start			(m_Nighvision, Actor(), false);

	if (m_pVision)
	{
		m_pVision->Update				();
		m_pVision->Draw					();
	}

	float scale							= g_pGamePersistent->m_pGShaderConstants->hud_params.z;
	Fvector2 pos						= { -g_pGamePersistent->m_pGShaderConstants->hud_params.x * Device.dwWidth, -g_pGamePersistent->m_pGShaderConstants->hud_params.y * Device.dwHeight };
	if (m_pUIReticle)
	{
		m_pUIReticle->SetScale			(scale);
		m_pUIReticle->SetWndPos			(pos);
		m_pUIReticle->Draw				();
	}

	scale								*= exp(pow(GetCurrentMagnification(), lense_circle_scale.z) * (lense_circle_scale.x + lense_circle_scale.y * (hud.HudOffset()[0].z - hud.HandsOffset(eScope)[0].z)));
	pUILenseCircle->SetScale			(scale);
	pUILenseCircle->SetWndPos			(pos.mul(lense_circle_offset.x * pow(GetCurrentMagnification(), lense_circle_offset.y)));
	pUILenseCircle->Draw				();

	Frect crect							= pUILenseCircle->GetWndRect();
	pUILenseBlackFill->SetWndRect		(Frect().set(0.f, 0.f, crect.right, crect.top));
	pUILenseBlackFill->Draw				();
	pUILenseBlackFill->SetWndRect		(Frect().set(crect.right, 0.f, UI_BASE_WIDTH, crect.bottom));
	pUILenseBlackFill->Draw				();
	pUILenseBlackFill->SetWndRect		(Frect().set(crect.left, crect.bottom, UI_BASE_WIDTH, UI_BASE_HEIGHT));
	pUILenseBlackFill->Draw				();
	pUILenseBlackFill->SetWndRect		(Frect().set(0.f, crect.top, crect.left, UI_BASE_HEIGHT));
	pUILenseBlackFill->Draw				();

	if (!HasLense())
	{
		pUILenseGlass->Draw				();
		pUILenseCircle->SetScale		(1.f);
		pUILenseCircle->SetWndPos		(Fvector2().set(0.f, 0.f));
		pUILenseCircle->Draw			();
	}
}

extern float aim_fov_tan;
float CScope::GetReticleScale(CWeaponHud CR$ hud) const
{
	switch (Type())
	{
		case eOptics:
			if (HasLense())
			{
				float lense_fov_tan = GetLenseRadius() / abs(hud.LenseOffset() - hud.HudOffset()[0].z);
				return lense_fov_tan / aim_fov_tan;
			}
			break;
		case eCollimator:
			return m_Magnificaion.current;
			break;
	}

	return 1.f;
}
