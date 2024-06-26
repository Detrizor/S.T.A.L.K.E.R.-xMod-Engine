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
		if (isEmptyChamber() && HudAnimationExist("anm_load_chamber"))
		{
			m_sub_state = eSubstateReloadChamber;
			PlayAnimReload();
		}
		else if (HudAnimationExist("anm_open"))
		{
			PlayHUDMotion				("anm_open", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndOpen", get_LastFP());
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
		if (!reloadCartridge() || (m_magazin.size() == m_magazin.capacity()) || !has_ammo_for_reload())
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
