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

	if (m_bTriStateReload = !!READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_reload", FALSE))
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
		if (m_magazin.size() == m_magazin.capacity() || !has_ammo_for_reload())
		{
			m_sub_state					= eSubstateReloadEnd;
			PlayAnimReload				();
		}
		else
		{
			PlayHUDMotion				("anm_add_cartridge", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAddCartridge", get_LastFP());
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
	case eSubstateReloadBoltLock:
		if (m_chamber.size())
		{
			CCartridge					chamber;
			unload_chamber				(&chamber);
			if (m_magazine)
				m_magazine->loadCartridge(chamber);
			else
				m_magazin.push_back		(chamber);
		}
		else
			unload_chamber				();
		inherited::PlayAnimReload		();
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
	case eSubstateReloadBoltLock:
		m_locked						= true;
		if (!m_actor || m_current_ammo)
		{
			m_sub_state					= eSubstateReloadInProcess;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadInProcess:
		if (!reloadCartridge())
			m_sub_state					= eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	case eSubstateReloadEnd:
		SwitchState						(eIdle);
		break;
	case eSubstateReloadChamber:
		inherited::OnAnimationEnd		(state);
		m_sub_state						= (m_magazin.size() != m_magazin.capacity() && has_ammo_for_reload()) ? eSubstateReloadInProcess : eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	default:
		inherited::OnAnimationEnd		(state);
	}
}

float CWeaponAutomaticShotgun::Aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eSyncData:
	{
		float res						= inherited::Aboba(type, data, param);
		auto se_obj						= (CSE_ALifeDynamicObject*)data;
		auto se_wpn						= smart_cast<CSE_ALifeItemWeaponAutoShotGun*>(se_obj);
		if (param)
		{
			se_wpn->m_AmmoIDs.clear		();
			for (auto& c : m_magazin)
				se_wpn->m_AmmoIDs.push_back(get_ammo_type(c.m_ammoSect));
		}
		else
		{
			for (int i = 0; i < se_wpn->m_AmmoIDs.size(); i++)
			{
				auto CR$ sect			= m_ammoTypes[se_wpn->m_AmmoIDs[i]];
				if (m_magazin[i].m_ammoSect != sect)
					m_magazin[i].Load	(*sect);
			}
		}
		return							res;
	}
	}

	return								inherited::Aboba(type, data, param);
}
