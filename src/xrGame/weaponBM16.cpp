#include "stdafx.h"
#include "weaponBM16.h"

CWeaponBM16::~CWeaponBM16()
{
}

void CWeaponBM16::Load	(LPCSTR section)
{
	inherited::Load		(section);
	m_sounds.LoadSound	(*HudSection(), "snd_reload_1", "sndReload1", true, m_eSoundReload);
}

bool CWeaponBM16::SingleCartridgeReload()
{
	if (ParentIsActor())
		return ((m_magazin.size() == 1) || (m_ammo_to_reload && m_ammo_to_reload->GetAmmoCount() == 1));
	else
	{
		bool b_both = HaveCartridgeInInventory(2);
		return ((m_magazin.size() == 1 || !b_both) && (m_set_next_ammoType_on_reload == undefined_ammo_type || m_ammoType == m_set_next_ammoType_on_reload));
	}
}

void CWeaponBM16::PlayAnimShoot()
{
	PlayHUDMotion						(shared_str().printf("anm_shot_%d", m_magazin.size() + 1), TRUE, GetState());
}

void CWeaponBM16::PlayAnimShow()
{
	PlayHUDMotion						(shared_str().printf("anm_show_%d", m_magazin.size()), TRUE, GetState());
}

void CWeaponBM16::PlayAnimHide()
{
	PlayHUDMotion						(shared_str().printf("anm_hide_%d", m_magazin.size()), TRUE, GetState());
}

void CWeaponBM16::PlayAnimBore()
{
	PlayHUDMotion						(shared_str().printf("anm_bore_%d", m_magazin.size()), TRUE, GetState());
}

void CWeaponBM16::PlayAnimReload()
{
	bool single							= SingleCartridgeReload();
	PlayHUDMotion						((single) ? "anm_reload_1" : "anm_reload_2", TRUE, GetState());
	PlaySound							((single) ? "sndReload1" : "sndReload", get_LastFP());
}

void  CWeaponBM16::PlayAnimIdleMoving()
{
	PlayHUDMotion						(shared_str().printf("anm_idle_moving_%d", m_magazin.size()), TRUE, GetState());
}

void  CWeaponBM16::PlayAnimIdleSprint()
{
	PlayHUDMotion						(shared_str().printf("anm_idle_sprint_%d", m_magazin.size()), TRUE, GetState());
}

void CWeaponBM16::PlayAnimIdle()
{
	if (TryPlayAnimIdle())
		return;

	LPCSTR mask							= (ADS()) ? "anm_idle_aim_%d" : "anm_idle_%d";
	PlayHUDMotion						(shared_str().printf(mask, m_magazin.size()), TRUE, GetState());
}
