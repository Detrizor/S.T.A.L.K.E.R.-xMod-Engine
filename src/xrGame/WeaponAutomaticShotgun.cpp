#include "stdafx.h"
#include "WeaponAutomaticShotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"
#include "addon.h"

void CWeaponAutomaticShotgun::Load(LPCSTR section)
{
	inherited::Load						(section);

	if (m_bTriStateReload = pSettings->r_bool_ex(section, "tri_state_reload", false))
	{
		LPCSTR hud_sect					= pSettings->r_string(cNameSect(), "hud");		//in case grip isn't loaded yet and we get hud_unusable instead of real hud from HudSection()
		m_sounds.LoadSound				(hud_sect, "snd_add_cartridge", "sndAddCartridge", false, m_eSoundAddCartridge);
		m_sounds.LoadSound				(hud_sect, "snd_close_weapon", "sndClose", false, m_eSoundClose);
		m_sounds.LoadSound				(hud_sect, "snd_open_weapon", "sndOpen", false, m_eSoundOpen);

		auto ao							= getModule<MAddonOwner>();
		m_loading_slot					= ao->emplaceSlot();
		m_loading_slot->setAttachBone	("loading");
		m_loading_slot->model_offset.c	= pSettings->r_dvector3(section, "loading_point");
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
		m_sub_state						= (cmd == kWPN_RELOAD && uncharged() && !m_locked) ? eSubstateReloadBoltPull : eSubstateReloadEnd;
		m_sounds.StopAllSounds			();
		drop_loading					(false);
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
		if (m_magazine && !m_magazine->Full() && has_ammo_for_reload())
		{
			attach_loading				(m_loading_slot);
			PlayHUDMotion				("anm_add_cartridge", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAddCartridge", get_LastFP());
		}
		else
		{
			drop_loading				(true);
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
		else if (m_locked && HudAnimationExist("anm_bolt_release"))
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
		if (!on_bolt_lock() && m_current_ammo)
		{
			m_sub_state					= eSubstateReloadInProcess;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadInProcess:
	{
		if (!m_loading_slot->empty())
		{
			auto loading				= m_loading_slot->addons.front();
			auto ammo					= loading->O.scast<CWeaponAmmo*>();
			ammo->Get					(m_cartridge, false);
		}
		m_magazine->loadCartridge		(m_cartridge);
		PlayAnimReload					();
		break;
	}
	case eSubstateReloadEnd:
		SwitchState						(eIdle);
		break;
	default:
		inherited::OnAnimationEnd		(state);
	}
}

void CWeaponAutomaticShotgun::drop_loading(bool destroy)
{
	if (m_bTriStateReload)
		detach_loading					(m_loading_slot, destroy);
	if (m_current_ammo)
	{
		if (!m_actor && unlimited_ammo())
			m_current_ammo->DestroyObject();
		m_current_ammo					= nullptr;
	}
}

void CWeaponAutomaticShotgun::OnHiddenItem()
{
	drop_loading						(false);
	inherited::OnHiddenItem				();
}

void CWeaponAutomaticShotgun::attach_loading(CAddonSlot* slot)
{
	if (slot->empty())
	{
		auto se_ammo					= giveItem(m_current_ammo->cNameSect().c_str(), m_current_ammo->GetCondition(), true);
		auto ammo						= Level().Objects.net_Find(se_ammo->ID);
		slot->attachAddon				(ammo->mcast<MAddon>());
	}
	m_current_ammo->ChangeAmmoCount		(-1);
	if (m_current_ammo->object_removed())
		m_current_ammo					= nullptr;
}

void CWeaponAutomaticShotgun::detach_loading(CAddonSlot* slot, bool destroy)
{
	if (!slot->empty())
	{
		auto loading					= slot->addons.front();
		slot->detachAddon				(loading, (destroy || !m_actor && unlimited_ammo()) ? 2 : 1);
	}
}

bool CWeaponAutomaticShotgun::tryChargeMagazine(CWeaponAmmo* ammo)
{
	if (m_grip)
	{
		m_current_ammo					= ammo;
		StartReload						(eSubstateReloadBegin);
		return							true;
	}
	return								false;
}
