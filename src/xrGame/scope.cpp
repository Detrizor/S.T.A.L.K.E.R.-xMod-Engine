#include "stdafx.h"
#include "player_hud.h"
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "weaponBinocularsVision.h"
#include "Torch.h"
#include "Actor.h"

#include "scope.h"

CUIStatic* pUILenseCircle		= NULL;
CUIStatic* pUILenseBlackFill	= NULL;
CUIStatic* pUILenseGlass		= NULL;

void createStatic(CUIStatic*& dest, LPCSTR texture, float mult = 1.f, EAlignment al = aCenter)
{
	dest						= xr_new<CUIStatic>();
	dest->InitTextureEx			(texture);
	Frect						rect;
	rect.set					(0.f, 0.f, mult * 1024.f, mult * 1024.f);
	dest->SetTextureRect		(rect);
	dest->SetWndSize			(Fvector2().set(mult * UI_BASE_HEIGHT, mult * UI_BASE_HEIGHT));
	dest->SetAlignment			(al);
	dest->SetAnchor				(al);
	dest->SetStretchTexture		(true);
}

CScope::CScope(CGameObject* obj) : CModule(obj)
{
	m_pUIReticle				= NULL;
	m_pVision					= NULL;
	m_pNight_vision				= NULL;
}

CScope::~CScope()
{
	xr_delete					(m_pUIReticle);
	xr_delete					(m_pVision);
	xr_delete					(m_pNight_vision);
}

void CScope::Load(LPCSTR section)
{
	m_Type						= (eScopeType)pSettings->r_u8(section, "type");

	switch (m_Type)
	{
	case eOptics:{
		m_Magnificaion.Load		(pSettings->r_string(section, "magnification"));
		m_Magnificaion.current	= m_Magnificaion.vmin;
		m_fLenseRadius			= pSettings->r_float(section, "lense_radius");
		m_Reticle				= pSettings->r_string(section, "reticle");
		m_AliveDetector			= pSettings->r_string(section, "alive_detector");
		m_Nighvision			= pSettings->r_string(section, "nightvision");
		InitVisors				();
	}break;
	case eCollimator:
		m_Magnificaion.Load		(pSettings->r_string(section, "reticle_scale"));
		if (m_Magnificaion.dynamic)
			m_Magnificaion.current = (m_Magnificaion.vmin + m_Magnificaion.vmax) / 2.f;
		break;
	}
}

bool CScope::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result					= false;
	if (Type() == eOptics)
	{
		result					|= CInventoryItem::process_if_exists(section, "reticle", m_Reticle, test);
		result					|= CInventoryItem::process_if_exists(section, "alive_detector", m_AliveDetector, test);
		result					|= CInventoryItem::process_if_exists(section, "nightvision", m_Nighvision, test);
		if (result)
			InitVisors			();
	}
	return						result;
}

void CScope::InitVisors()
{
	xr_delete					(m_pUIReticle);
	if (m_Reticle.size())
		createStatic			(m_pUIReticle, shared_str().printf("wpn\\reticle\\%s", m_Reticle).c_str());

	xr_delete(m_pVision);
	if (m_AliveDetector.size())
		m_pVision				= xr_new<CBinocularsVision>(m_AliveDetector);
	
	if (m_pNight_vision)
	{
		m_pNight_vision->Stop	(100000.f, false);
		xr_delete				(m_pNight_vision);
	}
	if (m_Nighvision.size())
		m_pNight_vision			= xr_new<CNightVisionEffector>(m_Nighvision);
}

void CScope::modify_holder_params(float &range, float &fov) const
{
	if (Type() == eOptics)
	{
		range					*= m_Magnificaion.vmax;
		fov						*= pow(m_Magnificaion.vmax, .25f);
	}
}

void CScope::ZoomChange(int val)
{
	m_Magnificaion.Shift		(val);
}

bool CScope::HasLense() const
{
	return						!fIsZero(m_fLenseRadius);
}

//extern Fvector cur_offs;
float CScope::ReticleCircleOffset(int idx, const hud_item_measures& m, Fvector* const hud_offset) const
{
	float offset				= lense_circle_offset[idx].x;//--xd temp * (hud_offset[0][idx] - m.m_hands_offset[0][eADS][idx]);
	//offset						+= lense_circle_offset[idx].y * (hud_offset[1][idx] - m.m_hands_offset[1][eADS][idx]);
	//offset						+= lense_circle_offset[idx].z * cur_offs[idx];
	offset						*= pow(GetCurrentMagnification(), lense_circle_offset[idx].w);
	return						offset;
}

void CScope::RenderUI(const hud_item_measures& m, Fvector* const hud_offset)
{
	
	//if (m_pNight_vision && !m_pNight_vision->IsActive())
       // m_pNight_vision->Start		(m_Nighvision, Actor(), false);		--xd на будущее, доделать

	if (m_pVision)
	{
		m_pVision->Update			();
		m_pVision->Draw				();
	}

	if (!pUILenseCircle)
	{
		createStatic				(pUILenseCircle, "wpn\\lense\\circle", 4.f);
		createStatic				(pUILenseBlackFill, "wpn\\lense\\black_fill", .125f, aLeftTop);
	}

	float scale						= exp(pow(GetCurrentMagnification(), lense_circle_scale.z));//--xd * (lense_circle_scale.x + lense_circle_scale.y * (hud_offset[0].z - m.m_hands_offset[0][eADS].z)));
	float reticle_scale				= GetReticleScale();
	pUILenseCircle->SetScale		(reticle_scale * scale);
	pUILenseCircle->SetX			(ReticleCircleOffset(0, m, hud_offset));
	pUILenseCircle->SetY			(ReticleCircleOffset(1, m, hud_offset));
	pUILenseCircle->Draw			();

	Frect crect						= pUILenseCircle->GetWndRect();
	pUILenseBlackFill->SetWndRect	(Frect().set(0.f, 0.f, crect.right, crect.top));
	pUILenseBlackFill->Draw			();
	pUILenseBlackFill->SetWndRect	(Frect().set(crect.right, 0.f, UI_BASE_WIDTH, crect.bottom));
	pUILenseBlackFill->Draw			();
	pUILenseBlackFill->SetWndRect	(Frect().set(crect.left, crect.bottom, UI_BASE_WIDTH, UI_BASE_HEIGHT));
	pUILenseBlackFill->Draw			();
	pUILenseBlackFill->SetWndRect	(Frect().set(0.f, crect.top, crect.left, UI_BASE_HEIGHT));
	pUILenseBlackFill->Draw			();

	if (m_pUIReticle)
	{
		m_pUIReticle->SetScale		(reticle_scale);
		m_pUIReticle->Draw			();
	}

	if (!HasLense())
	{
		if (!pUILenseGlass)
			createStatic			(pUILenseGlass, "wpn\\lense\\glass");
		pUILenseGlass->Draw			();
		pUILenseCircle->SetScale	(1.f);
		pUILenseCircle->SetWndPos	(Fvector2().set(0.f, 0.f));
		pUILenseCircle->Draw		();
	}
}
