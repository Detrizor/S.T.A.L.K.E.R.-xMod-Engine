#include "stdafx.h"
#include "WeaponAutomaticShotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"

CWeaponAutomaticShotgun::CWeaponAutomaticShotgun()
{
	m_eSoundClose					= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
	m_eSoundAddCartridge			= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
}

void CWeaponAutomaticShotgun::Load(LPCSTR section)
{
	inherited::Load					(section);

	m_bTriStateReload				= !!READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_reload", false);
	if (m_bTriStateReload)
	{
		m_sounds.LoadSound			(*HudSection(), "snd_open_weapon", "sndOpen", false, m_eSoundOpen);
		m_sounds.LoadSound			(*HudSection(), "snd_add_cartridge", "sndAddCartridge", false, m_eSoundAddCartridge);
		m_sounds.LoadSound			(*HudSection(), "snd_close_weapon", "sndClose", false, m_eSoundClose);
	}
}

bool CWeaponAutomaticShotgun::Action(u16 cmd, u32 flags) 
{
	if (inherited::Action				(cmd, flags))
		return							true;

	if (flags & CMD_START && m_bTriStateReload && GetState() == eReload &&
		(m_sub_state == eSubstateReloadInProcess || m_sub_state == eSubstateReloadBegin) &&
		(cmd == kWPN_FIRE || cmd == kWPN_RELOAD))		//остановить перезагрузку
	{
		m_sub_state						= (cmd == kWPN_RELOAD && isEmptyChamber()) ? eSubstateReloadBolt : eSubstateReloadEnd;
		m_sounds.StopAllSounds			();
		PlayAnimReload					();
		return							true;
	}
	return								false;
}

void CWeaponAutomaticShotgun::PlayAnimReload()
{
	if (!m_bTriStateReload)
		return							inherited::PlayAnimReload();

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		if (HudAnimationExist("anm_open"))
		{
			PlayHUDMotion				("anm_open", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndOpen", get_LastFP());
		}
		else if (isEmptyChamber() && HudAnimationExist("anm_load_chamber"))
		{
			m_sub_state					= eSubstateReloadChamber;
			PlayAnimReload				();
		}
		else
		{
			m_sub_state					= eSubstateReloadInProcess;
			PlayAnimReload				();
		}
		break;
	case eSubstateReloadInProcess:
		PlayHUDMotion					("anm_add_cartridge", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndAddCartridge", get_LastFP());
		break;
	case eSubstateReloadEnd:
		if (HudAnimationExist("anm_close"))
		{
			PlayHUDMotion				("anm_close", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndClose", get_LastFP());
		}
		else
			SwitchState					(eIdle);
		break;
	default:
		inherited::PlayAnimReload		();
	}
}

void CWeaponAutomaticShotgun::OnAnimationEnd(u32 state) 
{
	if (!m_bTriStateReload || state != eReload)
		return							inherited::OnAnimationEnd(state);

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		m_sub_state						= eSubstateReloadInProcess;
		PlayAnimReload					();
		break;
	case eSubstateReloadInProcess:
		if (!reloadCartridge() || m_magazin.size() == m_magazin.capacity() || !has_ammo_for_reload())
			m_sub_state					= eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	case eSubstateReloadEnd:
		reload_chamber					();
		SwitchState						(eIdle);
		break;
	case eSubstateReloadChamber:
		inherited::OnAnimationEnd		(state);
		m_sub_state						= eSubstateReloadInProcess;
		PlayAnimReload					();
		break;
	default:
		inherited::OnAnimationEnd		(state);
	}
}

void CWeaponAutomaticShotgun::net_Export(NET_Packet& P)
{
	inherited::net_Export			(P);	
	P.w_u8							(u8(m_magazin.size()));	
	for (u32 i = 0; i < m_magazin.size(); i++)
		P.w_u8						(get_ammo_type(m_magazin[i].m_ammoSect));
}

void CWeaponAutomaticShotgun::net_Import(NET_Packet& P)
{
	inherited::net_Import			(P);
	u8 AmmoCount = P.r_u8			();
	for (u32 i = 0; i < AmmoCount; i++)
	{
		u8 LocalAmmoType = P.r_u8	();
		if (i >= m_magazin.size())
			continue;
		CCartridge& l_cartridge		= m_magazin[i];
		if (m_ammoTypes[LocalAmmoType] == l_cartridge.m_ammoSect)
			continue;
		l_cartridge.Load			(m_ammoTypes[LocalAmmoType].c_str());
	}
}
