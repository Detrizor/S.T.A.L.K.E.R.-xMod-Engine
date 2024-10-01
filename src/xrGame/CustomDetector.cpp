#include "stdafx.h"
#include "customdetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "hudmanager.h"
#include "inventory.h"
#include "level.h"
#include "map_manager.h"
#include "ActorEffector.h"
#include "actor.h"
#include "ui/UIWindow.h"
#include "player_hud.h"
#include "weapon.h"

ITEM_INFO::ITEM_INFO()
{
	pParticle	= NULL;
	curr_ref	= NULL;
}

ITEM_INFO::~ITEM_INFO()
{
	if(pParticle)
		CParticlesObject::Destroy(pParticle);
}

bool CCustomDetector::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
{
	if(itm==NULL)
		return true;

	CInventoryItem& iitm			= itm->item();
	u32 slot						= iitm.BaseSlot();
	bool bres = (slot == BOLT_SLOT || slot == KNIFE_SLOT || slot == PISTOL_SLOT);
	if(!bres && slot_to_activate)
	{
		*slot_to_activate	= NO_ACTIVE_SLOT;
		if(m_pInventory->ItemFromSlot(BOLT_SLOT))
			*slot_to_activate = BOLT_SLOT;

		if(m_pInventory->ItemFromSlot(KNIFE_SLOT))
			*slot_to_activate = KNIFE_SLOT;

		if (m_pInventory->ItemFromSlot(PISTOL_SLOT))
			*slot_to_activate = PISTOL_SLOT;

		if(*slot_to_activate != NO_ACTIVE_SLOT)
			bres = true;
	}

	if(itm->GetState()!=CHUDState::eShowing)
		bres = bres && !itm->IsPending();

	if(bres)
	{
		CWeapon* W = smart_cast<CWeapon*>(itm);
		if(W)
			bres =	bres								&& 
					(W->GetState()!=CHUDState::eBore)	&& 
					(W->GetState()!=CWeapon::eReload) && 
					(W->GetState()!=CWeapon::eSwitch) && 
					!W->IsZoomed();
	}
	return bres;
}

bool CCustomDetector::CheckCompatibility(CHudItem* itm)
{
	return (inherited::CheckCompatibility(itm)) ? CheckCompatibilityInt(itm, nullptr) : false;
}

void CCustomDetector::OnActiveItem()
{
	show(!!g_player_hud->attached_item(0));
}

void CCustomDetector::OnHiddenItem()
{
	hide(!!g_player_hud->attached_item(0));
}

void CCustomDetector::show(bool bFastMode)
{
	if (toggle(true, bFastMode))
		inherited::OnActiveItem();
}

void CCustomDetector::hide(bool bFastMode)
{
	if (toggle(false, bFastMode))
		inherited::OnHiddenItem();
}

bool CCustomDetector::toggle(bool status, bool bFastMode)
{
	m_bNeedActivation					= false;
	m_bFastAnimMode						= bFastMode;

	if (status)
	{
		PIItem iitem					= m_pInventory->ActiveItem();
		CHudItem* itm					= (iitem)?iitem->cast_hud_item():NULL;
		u16 slot_to_activate			= NO_ACTIVE_SLOT;

		if (CheckCompatibilityInt(itm, &slot_to_activate))
		{
			if (slot_to_activate != NO_ACTIVE_SLOT)
			{
				m_pInventory->Activate	(slot_to_activate);
				m_bNeedActivation		= true;
			}
			else
				return					true;
		}
	}
	else
		return							true;
}

void CCustomDetector::OnStateSwitch(u32 S, u32 oldState)
{
	inherited::OnStateSwitch			(S, oldState);

	switch(S)
	{
	case eShowing:
		g_player_hud->attach_item		(this);
		m_sounds.PlaySound				("sndShow", Fvector().set(0,0,0), this, true, false);
		PlayHUDMotion					(m_bFastAnimMode?"anm_show_fast":"anm_show", FALSE/*TRUE*/, GetState());
		SetPending						(TRUE);
		break;
	case eHiding:
		if (oldState != eHiding)
		{
			m_sounds.PlaySound			("sndHide", Fvector().set(0, 0, 0), this, true, false);
			PlayHUDMotion				(m_bFastAnimMode ? "anm_hide_fast" : "anm_hide", FALSE/*TRUE*/, GetState());
			SetPending					(TRUE);
			TurnDetectorInternal		(false);
		}
		break;
	case eIdle:
		TurnDetectorInternal			(true);
		break;
	}
}

void CCustomDetector::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

CCustomDetector::CCustomDetector() 
{
	m_bFastAnimMode		= false;
	m_bNeedActivation	= false;
}

CCustomDetector::~CCustomDetector() 
{
	m_artefacts.destroy	();
}

BOOL CCustomDetector::net_Spawn(CSE_Abstract* DC) 
{
	CreateUI			();
	return		(inherited::net_Spawn(DC));
}

void CCustomDetector::Load(LPCSTR section) 
{
	inherited::Load			(section);

	m_fAfDetectRadius		= pSettings->r_float(section,"af_radius");
	m_fAfVisRadius			= pSettings->r_float(section,"af_vis_radius");
	m_artefacts.load		(section, "af");

	m_sounds.LoadSound( section, "snd_draw", "sndShow");
	m_sounds.LoadSound( section, "snd_holster", "sndHide");
}

void CCustomDetector::shedule_Update(u32 dt) 
{
	inherited::shedule_Update(dt);
	
	if( !IsWorking() )			return;

	Position().set(H_Parent()->Position());

	Fvector						P; 
	P.set						(H_Parent()->Position());

	if (IsUsingCondition() && GetCondition() <= 0.01f)
		return;

	m_artefacts.feel_touch_update(P,m_fAfDetectRadius);
}


bool CCustomDetector::IsWorking()
{
	return m_bWorking && H_Parent() && H_Parent()==Level().CurrentViewEntity();
}

void CCustomDetector::UpfateWork()
{
	UpdateAf				();
	m_ui->update			();
}

void CCustomDetector::UpdateVisibility()
{
	//check visibility
	attachable_hud_item* i0		= g_player_hud->attached_item(0);
	if(i0 && HudItemData())
	{
		bool bClimb			= ( (Actor()->MovingState()&mcClimb) != 0 );
		if(bClimb)
		{
			hide				(true);
			m_bNeedActivation	= true;
		}else
		{
			CWeapon* wpn			= smart_cast<CWeapon*>(i0->m_parent_hud_item);
			if(wpn)
			{
				u32 state			= wpn->GetState();
				if(wpn->IsZoomed() || state==CWeapon::eReload || state==CWeapon::eSwitch)
				{
					hide				(true);
					m_bNeedActivation	= true;
				}
			}
		}
	}else
	if(m_bNeedActivation)
	{
		attachable_hud_item* i0		= g_player_hud->attached_item(0);
		bool bClimb					= ( (Actor()->MovingState()&mcClimb) != 0 );
		if(!bClimb)
		{
			CHudItem* huditem		= (i0)?i0->m_parent_hud_item : NULL;
			bool bChecked			= !huditem || CheckCompatibilityInt(huditem, 0);
			
			if(	bChecked )
				show				(true);
		}
	}
}

void CCustomDetector::UpdateCL() 
{
	inherited::UpdateCL();

	if(H_Parent()!=Level().CurrentEntity() )			return;

	UpdateVisibility		();
	if( !IsWorking() )		return;
	UpfateWork				();
}

void CCustomDetector::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
	m_artefacts.clear			();
	TurnDetectorInternal		(false);
}

bool CAfList::feel_touch_contact	(CObject* O)
{
	TypesMapIt it				= m_TypesMap.find(O->cNameSect());

	bool res					 = (it!=m_TypesMap.end());
	if(res)
	{
		CArtefact*	pAf				= smart_cast<CArtefact*>(O);
		
		if(pAf->GetAfRank()>m_af_rank)
			res = false;
	}
	return						res;
}

bool CCustomDetector::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);

	result		|= process_if_exists(section,	"af_radius",		m_fAfDetectRadius,	test);
	result		|= process_if_exists(section,	"af_vis_radius",	m_fAfVisRadius,		test);

	return result;
}
