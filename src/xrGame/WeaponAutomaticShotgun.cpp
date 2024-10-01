#include "stdafx.h"
#include "WeaponAutomaticShotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"

void CWeaponAutomaticShotgun::Load(LPCSTR section)
{
	inherited::Load						(section);

	if (m_bTriStateReload = !!READ_IF_EXISTS(pSettings, r_BOOL, section, "tri_state_reload", FALSE))
	{
		LPCSTR hud_sect					= pSettings->r_string(cNameSect(), "hud");		//in case grip isn't loaded yet and we get hud_unusable instead of real hud from HudSection()
		m_sounds.LoadSound				(hud_sect, "snd_add_cartridge", "sndAddCartridge", false, m_eSoundAddCartridge);
		m_sounds.LoadSound				(hud_sect, "snd_close_weapon", "sndClose", false, m_eSoundClose);
		m_sounds.LoadSound				(hud_sect, "snd_open_weapon", "sndOpen", false, m_eSoundOpen);
	}
}

bool CWeaponAutomaticShotgun::Action(u16 cmd, u32 flags) 
{
	if (inherited::Action				(cmd, flags))
		return							true;

	if (m_bTriStateReload && flags&CMD_START && GetState() == eReload &&
		(m_sub_state == eSubstateReloadInProcess || m_sub_state == eSubstateReloadBegin) &&
		(cmd == kWPN_FIRE || cmd == kWPN_RELOAD))		//остановить перезагрузку
	{
		m_sub_state						= (cmd == kWPN_RELOAD && isEmptyChamber() && !m_locked) ? eSubstateReloadBoltPull : eSubstateReloadEnd;
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
		if (isEmptyChamber() && HudAnimationExist("anm_load_chamber"))
		{
			m_sub_state					= eSubstateReloadChamber;
			PlayAnimReload				();
		}
		else if (HudAnimationExist("anm_open"))
		{
			PlayHUDMotion				("anm_open", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndOpen", get_LastFP());
		}
		else if (HudAnimationExist("anm_bolt_lock") && !m_locked)
		{
			m_sub_state					= eSubstateReloadBoltLock;
			PlayAnimReload				();
		}
		else
		{
			m_sub_state					= eSubstateReloadInProcess;
			PlayAnimReload				();
		}
		break;
	case eSubstateReloadInProcess:
		if (can_load_cartridge())
		{
			PlayHUDMotion				("anm_add_cartridge", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAddCartridge", get_LastFP());
		}
		else
		{
			m_sub_state					= eSubstateReloadEnd;
			PlayAnimReload				();
		}
		break;
	case eSubstateReloadEnd:
		if (HudAnimationExist("anm_close"))
		{
			PlayHUDMotion				("anm_close", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndClose", get_LastFP());
		}
		else if (HudAnimationExist("anm_bolt_release"))
		{
			m_sub_state					= eSubstateReloadBoltRelease;
			inherited::PlayAnimReload	();
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
	case eSubstateReloadBoltLock:
		m_locked						= true;
		if (m_chamber.size() && m_magazine)
		{
			unload_chamber				(&m_cartridge);
			m_magazine->loadCartridge	(m_cartridge);
		}
		else
			unload_chamber				();
		if (!m_actor || m_current_ammo)
		{
			m_sub_state					= eSubstateReloadInProcess;
			PlayAnimReload				();
		}
		else if (m_magazine_slot && m_magazine_slot->isLoading())
		{
			m_sub_state					= eSubstateReloadAttach;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadInProcess:
		if (!reload_сartridge())
			m_sub_state					= eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	case eSubstateReloadEnd:
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

bool CWeaponAutomaticShotgun::can_load_cartridge() const
{
	return								m_magazine && !m_magazine->Full() && has_ammo_for_reload();
}

bool CWeaponAutomaticShotgun::is_dummy_anm() const
{
	return								!m_shot_shell && (m_sub_state == eSubstateReloadBoltPull && m_chamber.empty() || m_sub_state == eSubstateReloadBoltLock);
}
