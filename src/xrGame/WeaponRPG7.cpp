#include "stdafx.h"
#include "weaponrpg7.h"
#include "xrserver_objects_alife_items.h"
#include "explosiverocket.h"
#include "entity.h"
#include "level.h"
#include "player_hud.h"
#include "hudmanager.h"

void CWeaponRPG7::Load	(LPCSTR section)
{
	inherited::Load						(section);
	CRocketLauncher::Load				(section);
	m_sRocketSection					= pSettings->r_string	(section,"rocket_class");
}

BOOL CWeaponRPG7::net_Spawn(CSE_Abstract* DC) 
{
	BOOL l_res = inherited::net_Spawn(DC);

	if (m_chamber.loaded() && !getCurrentRocket())
		CRocketLauncher::SpawnRocket(m_sRocketSection, this);

	return l_res;
}

void CWeaponRPG7::ReloadMagazine() 
{
	inherited::ReloadMagazine();
	if (m_chamber.loaded() && !getRocketCount())
		CRocketLauncher::SpawnRocket(m_sRocketSection.c_str(), this);
}

#include "inventory.h"
#include "inventoryOwner.h"
void CWeaponRPG7::switch2_Fire()
{
	m_iShotNum			= 0;
	m_bFireSingleShot	= true;
	bWorking			= false;

	if(GetState()==eFire && getRocketCount()) 
	{
		Fvector p1, d1, p; 
		Fvector p2, d2, d; 
		p1.set								(get_LastFP()); 
		d1.set								(get_LastFD());
		p = p1;
		d = d1;
		CEntity* E = smart_cast<CEntity*>	(H_Parent());
		if(E)
		{
			E->g_fireParams				(this, p2,d2);
			p = p2;
			d = d2;

			if(IsHudModeNow())
			{
				Fvector		p0;
				float dist	= HUD().GetCurrentRayQuery().range;
				p0.mul		(d2,dist);
				p0.add		(p1);
				p			= p1;
				d.sub		(p0,p1);
				d.normalize_safe();
			}
		}

		Fmatrix								launch_matrix;
		launch_matrix.identity				();
		launch_matrix.k.set					(d);
		Fvector::generate_orthonormal_basis(launch_matrix.k,
											launch_matrix.j, launch_matrix.i);
		launch_matrix.c.set					(p);

		d.normalize							();
		d.mul								(m_fLaunchSpeed);

		CRocketLauncher::LaunchRocket		(launch_matrix, d, zero_vel);

		CExplosiveRocket* pGrenade			= smart_cast<CExplosiveRocket*>(getCurrentRocket());
		VERIFY								(pGrenade);
		pGrenade->SetInitiator				(H_Parent()->ID());

		if (OnServer())
		{
			NET_Packet						P;
			u_EventGen						(P,GE_LAUNCH_ROCKET,ID());
			P.w_u16							(u16(getCurrentRocket()->ID()));
			u_EventSend						(P);
		}
	}
}

void CWeaponRPG7::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent(P,type);
	u16 id;
	switch (type)
	{
		case GE_OWNERSHIP_TAKE:
			P.r_u16(id);
			CRocketLauncher::AttachRocket(id, this);
			break;
		case GE_OWNERSHIP_REJECT:
		case GE_LAUNCH_ROCKET:
		{
			bool bLaunch = (type == GE_LAUNCH_ROCKET);
			P.r_u16(id);
			CRocketLauncher::DetachRocket(id);
			break;
		}
	}
}
