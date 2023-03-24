#include "stdafx.h"
#include "EliteDetector.h"
#include "player_hud.h"
#include "../Include/xrRender/UIRender.h"
#include "ui/UIXmlInit.h"
#include "ui/xrUIXmlParser.h"
#include "ui/UIStatic.h"
#include "ui/ArtefactDetectorUI.h"



CEliteDetector::CEliteDetector()
{
	m_artefacts.m_af_rank = 3;
}

CEliteDetector::~CEliteDetector()
{}


void CEliteDetector::CreateUI()
{
	R_ASSERT(NULL==m_ui);
	m_ui				= xr_new<CUIArtefactDetectorElite>();
	ui().construct		(this);
}

CUIArtefactDetectorElite&  CEliteDetector::ui()
{
	return *((CUIArtefactDetectorElite*)m_ui);
}

void CEliteDetector::UpdateAf()
{
	ui().Clear							();
	if(m_artefacts.m_ItemInfos.size()==0)	return;

	CAfList::ItemsMapIt it_b	= m_artefacts.m_ItemInfos.begin();
	CAfList::ItemsMapIt it_e	= m_artefacts.m_ItemInfos.end();
	CAfList::ItemsMapIt it		= it_b;

	Fvector						detector_pos = Position();
	for(;it_b!=it_e;++it_b)
	{
		CArtefact *pAf			= it_b->first;
		if(pAf->H_Parent())		
			continue;

		ui().RegisterItemToDraw			(pAf->Position(),"af_sign");

		if(pAf->CanBeInvisible())
		{
			float d = detector_pos.distance_to(pAf->Position());
			if(d<m_fAfVisRadius)
				pAf->SwitchVisibility(true);
		}
	}
}

bool  CEliteDetector::render_item_3d_ui_query()
{
	return IsWorking();
}

void CEliteDetector::render_item_3d_ui()
{
	R_ASSERT(HudItemData());
	inherited::render_item_3d_ui();
	ui().Draw			();
	//	Restore cull mode
	UIRender->CacheSetCullMode	(IUIRender::cmCCW);
}

void CUIArtefactDetectorElite::construct(CEliteDetector* p)
{
	m_parent							= p;
	CUIXml								uiXml;
	uiXml.Load							(CONFIG_PATH, UI_PATH, "ui_detector_artefact.xml");

	CUIXmlInit							xml_init;
	string512							buff;
	xr_strcpy							(buff, p->ui_xml_tag());

	xml_init.InitWindow					(uiXml, buff, 0, this);

	m_wrk_area							= xr_new<CUIWindow>();

	xr_sprintf							(buff, "%s:wrk_area", p->ui_xml_tag());

	xml_init.InitWindow					(uiXml, buff, 0, m_wrk_area);
	m_wrk_area->SetAutoDelete			(true);
	AttachChild							(m_wrk_area);

	xr_sprintf							(buff, "%s", p->ui_xml_tag());
	int num = uiXml.GetNodesNum			(buff,0,"palette");
	XML_NODE* pStoredRoot				= uiXml.GetLocalRoot();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode(buff,0));
	for(int idx=0; idx<num;++idx)
	{
		CUIStatic* S					= xr_new<CUIStatic>();
		shared_str name					= uiXml.ReadAttrib("palette",idx,"id");
		m_palette[name]					= S;
		xml_init.InitStatic				(uiXml, "palette", idx, S);
		S->SetAutoDelete				(true);
		m_wrk_area->AttachChild			(S);
		S->SetCustomDraw				(true);
	}
	uiXml.SetLocalRoot					(pStoredRoot);

	Fvector _map_attach_p				= pSettings->r_fvector3(m_parent->cNameSect(), "ui_p");
	Fvector _map_attach_r				= pSettings->r_fvector3(m_parent->cNameSect(), "ui_r");
	
	_map_attach_r.mul					(PI/180.f);
	m_map_attach_offset.setHPB			(_map_attach_r.x, _map_attach_r.y, _map_attach_r.z);
	m_map_attach_offset.translate_over	(_map_attach_p);
}

void CUIArtefactDetectorElite::update()
{
	inherited::update();
	CUIWindow::Update();
}

void CUIArtefactDetectorElite::Draw()
{

	Fmatrix						LM;
	GetUILocatorMatrix			(LM);

	IUIRender::ePointType bk	= UI().m_currentPointType;

	UI().m_currentPointType	= IUIRender::pttLIT;

	UIRender->CacheSetXformWorld(LM);
	UIRender->CacheSetCullMode	(IUIRender::cmNONE);

	CUIWindow::Draw				();

	Fvector2 wrk_sz				= m_wrk_area->GetWndSize();
	Fvector2					rp; 
	m_wrk_area->GetAbsolutePos	(rp);

	Fmatrix						M, Mc;
	float h,p;
	Device.vCameraDirection.getHP(h,p);
	Mc.setHPB					(h,0,0);
	Mc.c.set					(Device.vCameraPosition);
	M.invert					(Mc);

	UI().ScreenFrustumLIT().CreateFromRect(Frect().set(	rp.x,
													rp.y,
													wrk_sz.x,
													wrk_sz.y ));

	xr_vector<SDrawOneItem>::const_iterator it	 = m_items_to_draw.begin();
	xr_vector<SDrawOneItem>::const_iterator it_e = m_items_to_draw.end();
	for(;it!=it_e;++it)
	{
		Fvector					p = (*it).pos;
		Fvector					pt3d;
		M.transform_tiny		(pt3d,p);
		float kz				= wrk_sz.y / m_parent->m_fAfDetectRadius;
		pt3d.x					*= kz;
		pt3d.z					*= kz;

		pt3d.x					+= wrk_sz.x/2.0f;	
		pt3d.z					-= wrk_sz.y;

		Fvector2				pos;
		pos.set					(pt3d.x, -pt3d.z);
		pos.sub					(rp);
		if(1 /* r.in(pos)*/ )
		{
			(*it).pStatic->SetWndPos	(pos);
			(*it).pStatic->Draw			();
		}
	}

	UI().m_currentPointType		= bk;
}

void CUIArtefactDetectorElite::GetUILocatorMatrix(Fmatrix& _m)
{
	Fmatrix	trans					= m_parent->HudItemData()->m_item_transform;
	u16 bid							= m_parent->HudItemData()->m_model->LL_BoneID("cover");
	Fmatrix cover_bone				= m_parent->HudItemData()->m_model->LL_GetTransform(bid);
	_m.mul							(trans, cover_bone);
	_m.mulB_43						(m_map_attach_offset);
}


void CUIArtefactDetectorElite::Clear()
{
	m_items_to_draw.clear();
}

void CUIArtefactDetectorElite::RegisterItemToDraw(const Fvector& p, const shared_str& palette_idx)
{
	xr_map<shared_str,CUIStatic*>::iterator it = m_palette.find(palette_idx);
	if(it==m_palette.end())		
	{
		Msg("! RegisterItemToDraw. static not found for [%s]", palette_idx.c_str());
		return;
	}
	CUIStatic* S				= m_palette[palette_idx];
	SDrawOneItem				itm(S, p);
	m_items_to_draw.push_back	(itm);
}

CScientificDetector::CScientificDetector()
{
	m_artefacts.m_af_rank		= 3;
	bZonesEnabled				= true;
	m_iAfDetectRadiusCurPower	= 0;
}

CScientificDetector::~CScientificDetector()
{
	m_zones.destroy();
}

void  CScientificDetector::Load(LPCSTR section)
{
	inherited::Load					(section);
	m_zones.load					(section,"zone");
	m_sounds.LoadSound				(section, "snd_toggle", "sndToggle");
	m_fAfDetectRadiusBase			= pSettings->r_float(section, "af_radius");
	m_fAfVisRadiusBase				= pSettings->r_float(section, "af_vis_radius");
	m_fAfDetectRadiusFactor			= pSettings->r_float(section, "af_radius_factor");
	m_iAfDetectRadiusPowersNum		= pSettings->r_u8(section, "af_radius_powers_num");
}

void CScientificDetector::UpfateWork()
{
	ui().Clear				();
	Fvector detector_pos	= Position();

	for (CAfList::ItemsMapIt ait = m_artefacts.m_ItemInfos.begin(), ait_e = m_artefacts.m_ItemInfos.end(); ait != ait_e; ++ait)
	{
		CArtefact* pAf					= ait->first;
		if (pAf->H_Parent())		
			continue;

		ui().RegisterItemToDraw			(pAf->Position(), pAf->cNameSect());
		if (pAf->CanBeInvisible())
		{
			float d						= detector_pos.distance_to(pAf->Position());
			if (d < m_fAfVisRadius)
				pAf->SwitchVisibility	(true);
		}
	}

	if (bZonesEnabled)
	{
		for (CZoneList::ItemsMapIt zit = m_zones.m_ItemInfos.begin(), zit_e = m_zones.m_ItemInfos.end(); zit != zit_e; ++zit)
		{
			CCustomZone* pZone			= zit->first;
			ui().RegisterItemToDraw		(pZone->Position(), pZone->cNameSect());
		}
	}

	m_ui->update();
}

void CScientificDetector::shedule_Update(u32 dt) 
{
	inherited::shedule_Update	(dt);

	if(!H_Parent())				return;
	Fvector						P; 
	P.set						(H_Parent()->Position());
	m_zones.feel_touch_update	(P,m_fAfDetectRadius);
}

void CScientificDetector::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
	
	m_zones.clear			();
}

bool CScientificDetector::Action(u16 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags))
		return true;

	switch (cmd)
	{
	case kWPN_ZOOM_INC:
	case kWPN_ZOOM_DEC:
		if (flags&CMD_START)
		{
			if (cmd == kWPN_ZOOM_INC)
				ChangeLevel		(true);
			else
				ChangeLevel		(false);
			return				true;
		}
		else
			return				false;
	}

	return false;
}

void CScientificDetector::ChangeLevel(bool increase)
{
	bool changed						= false;
	if (increase)
	{
		if (m_iAfDetectRadiusCurPower != m_iAfDetectRadiusPowersNum)
		{
			m_iAfDetectRadiusCurPower	+= 1;
			changed						= true;
		}
	}
	else if (m_iAfDetectRadiusCurPower != 0)
	{
		m_iAfDetectRadiusCurPower		-= 1;
		changed							= true;
	}

	if (changed)
	{
		m_fAfDetectRadius		= m_fAfDetectRadiusBase * (float)pow(m_fAfDetectRadiusFactor, m_iAfDetectRadiusCurPower);
		m_fAfVisRadius			= (m_iAfDetectRadiusCurPower == 0) ? m_fAfVisRadiusBase : 0.f;
		bZonesEnabled			= (m_iAfDetectRadiusCurPower == 0);
		m_sounds.PlaySound		("sndToggle", Fvector().set(0, 0, 0), this, true, false);
	}
}

bool CScientificDetector::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);
	
	result		|= process_if_exists(section,	"af_radius",				m_fAfDetectRadiusBase,			test);
	result		|= process_if_exists(section,	"af_vis_radius",			m_fAfVisRadiusBase,				test);
	result		|= process_if_exists(section,	"af_radius_factor",			m_fAfDetectRadiusFactor,		test);
	result		|= process_if_exists(section,	"af_radius_powers_num",		m_iAfDetectRadiusPowersNum,		test);

	return result;
}