#include "stdafx.h"
#include "grenade.h"
#include "../xrphysics/PhysicsShell.h"
//.#include "WeaponHUD.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "actor.h"
#include "inventory.h"
#include "level.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife.h"

#define GRENADE_REMOVE_TIME		30000
const float default_grenade_detonation_threshold_hit=100;
CGrenade::CGrenade(void) 
{

	m_destroy_callback.clear();
	m_eSoundCheckout = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
}

void CGrenade::Load(LPCSTR section) 
{
	inherited::Load(section);
	CExplosive::Load(section);

	m_sounds.LoadSound(section,"snd_checkout", "sndCheckout", false, m_eSoundCheckout);

	//////////////////////////////////////
	//����� �������� ������ � ������
	if(pSettings->line_exist(section,"grenade_remove_time"))
		m_dwGrenadeRemoveTime = pSettings->r_u32(section,"grenade_remove_time");
	else
		m_dwGrenadeRemoveTime = GRENADE_REMOVE_TIME;
	m_grenade_detonation_threshold_hit=READ_IF_EXISTS(pSettings,r_float,section,"detonation_threshold_hit",default_grenade_detonation_threshold_hit);
}

void CGrenade::Hit					(SHit* pHDS)
{
	if( ALife::eHitTypeExplosion==pHDS->hit_type && m_grenade_detonation_threshold_hit<pHDS->damage()&&CExplosive::Initiator()==u16(-1)) 
	{
		CExplosive::SetCurrentParentID(pHDS->who->ID());
		Destroy();
	}
	inherited::Hit(pHDS);
}

BOOL CGrenade::net_Spawn(CSE_Abstract* DC) 
{
	m_dwGrenadeIndependencyTime			= 0;
	BOOL ret= inherited::net_Spawn		(DC);
	Fvector box;BoundingBox().getsize	(box);
	float max_size						= _max(_max(box.x,box.y),box.z);
	box.set								(max_size,max_size,max_size);
	box.mul								(3.f);
	CExplosive::SetExplosionSize		(box);
	m_thrown							= false;
	return								ret;
}

void CGrenade::net_Destroy() 
{
	if(m_destroy_callback)
	{
		m_destroy_callback				(this);
		m_destroy_callback				= destroy_callback(NULL);
	}

	inherited::net_Destroy				();
	CExplosive::net_Destroy				();
}

void CGrenade::OnH_A_Independent() 
{
	m_dwGrenadeIndependencyTime			= Level().timeServer();
	inherited::OnH_A_Independent		();	
}

void CGrenade::OnH_A_Chield()
{
	m_dwGrenadeIndependencyTime			= 0;
	m_dwDestroyTime						= 0xffffffff;
	inherited::OnH_A_Chield				();
}

void CGrenade::State(u32 state, u32 old_state) 
{
	switch (state)
	{
	case eThrowStart:
		{
			Fvector						C;
			Center						(C);
			PlaySound					("sndCheckout", C);
		}break;
	case eThrowEnd:
		{
			if(m_thrown)
			{
				if (m_pPhysicsShell)
					m_pPhysicsShell->Deactivate();
				xr_delete	( m_pPhysicsShell );
				m_dwDestroyTime			= 0xffffffff;
				PutNextToSlot			();
				if (Local())
				{
#ifndef MASTER_GOLD
					Msg( "Destroying local grenade[%d][%d]", ID(), Device.dwFrame );
#endif // #ifndef MASTER_GOLD
					DestroyObject();
				}
				
			};
		}break;
	};
	inherited::State( state, old_state );
}

bool CGrenade::DropGrenade()
{
	EMissileStates grenade_state = static_cast<EMissileStates>(GetState());
	if (((grenade_state == eThrowStart) ||
		(grenade_state == eReady) ||
		(grenade_state == eThrow)) &&
		(!m_thrown)
		)
	{
		Throw();
		return true;
	}
	return false;
}

void CGrenade::DiscardState()
{	
	u32 state = GetState();
	if (state==eReady || state==eThrow)
	{
		OnStateSwitch(eIdle, state);
	}
}

void CGrenade::Throw() 
{
	if (m_thrown)
		return;

	if (!m_fake_missile)
		return;

	CGrenade					*pGrenade = smart_cast<CGrenade*>( m_fake_missile );
	VERIFY						(pGrenade);
	
	if (pGrenade) 
	{
		pGrenade->set_destroy_time(m_dwDestroyTimeMax);
//���������� ID ���� ��� ����� �������
		pGrenade->SetInitiator( H_Parent()->ID() );
	}
	inherited::Throw			();
	m_fake_missile->processing_activate();//@sliph
	m_thrown = true;
}

void CGrenade::Destroy() 
{
	//Generate Expode event
	Fvector						normal;

	if(m_destroy_callback)
	{
		m_destroy_callback		(this);
		m_destroy_callback	=	destroy_callback(NULL);
	}

	FindNormal					(normal);
	CExplosive::GenExplodeEvent	(Position(), normal);
}

bool CGrenade::Useful() const
{

	bool res = (/* !m_throw && */ m_dwDestroyTime == 0xffffffff && CExplosive::Useful() && TestServerFlag(CSE_ALifeObject::flCanSave));

	return res;
}

void CGrenade::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent			(P,type);
	CExplosive::OnEvent			(P,type);
}

void CGrenade::PutNextToSlot()
{
	VERIFY								(!getDestroy());
	//�������� ������� �� ���������
	if (m_pInventory)
	{
		m_pInventory->Ruck				(this);
		m_thrown						= false;
	}
	else
		Msg								("! PutNextToSlot : m_pInventory = NULL [%d][%d]", ID(), Device.dwFrame);	
}

void CGrenade::OnAnimationEnd(u32 state) 
{
	switch(state)
	{
	case eThrowEnd: SwitchState(eHidden);	break;
	default : inherited::OnAnimationEnd(state);
	}
}

void CGrenade::UpdateCL() 
{
	inherited::UpdateCL			();
	CExplosive::UpdateCL		();
}

ALife::_TIME_ID	 CGrenade::TimePassedAfterIndependant()	const
{
	if(!H_Parent() && m_dwGrenadeIndependencyTime != 0)
		return Level().timeServer() - m_dwGrenadeIndependencyTime;
	else
		return 0;
}

BOOL CGrenade::UsedAI_Locations		()
{
#pragma todo("Dima to Yura : It crashes, because on net_Spawn object doesn't use AI locations, but on net_Destroy it does use them")
	return inherited::UsedAI_Locations( );//m_dwDestroyTime == 0xffffffff;
}

void CGrenade::net_Relcase(CObject* O )
{
	CExplosive::net_Relcase(O);
	inherited::net_Relcase(O);
}

void CGrenade::DeactivateItem(u16 slot)
{
	//Drop grenade if primed
	StopCurrentAnimWithoutCallback();
	if ( !GetTmpPreDestroy() && Local() && ( GetState()==eThrowStart || GetState()==eReady || GetState()==eThrow ) )
	{
		if (m_fake_missile)
		{
			CGrenade*		pGrenade	= smart_cast<CGrenade*>( m_fake_missile );
			if ( pGrenade )
			{
				if ( m_pInventory->GetOwner() )
				{
					CActor* pActor = smart_cast<CActor*>( m_pInventory->GetOwner() );
					if (pActor)
					{
						if ( !pActor->g_Alive() )
						{
							m_constpower			= false;
							m_fThrowForce			= 0;
						}
					}
				}				
				Throw	();
			};
		};
	};

	inherited::DeactivateItem(slot);
}

void CGrenade::onMovementChanged()
{
	if (GetState() < eThrowStart || GetState() > eThrowEnd)
		__super::onMovementChanged();
}
