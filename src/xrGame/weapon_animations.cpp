#include "stdafx.h"
#include "WeaponMagazined.h"

void CWeaponMagazined::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadDetach:
		PlayHUDMotion					("anm_detach", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndDetach", get_LastFP());
		break;
	case eSubstateReloadAttach:
		m_magazine_slot->loadingAttach	();
		PlayHUDMotion					("anm_attach", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndAttach", get_LastFP());
		break;
	case eSubstateReloadBolt:
	{
		bool ex							= HudAnimationExist("anm_bolt_pull");
		if (m_locked || !ex)
		{
			PlayHUDMotion				("anm_bolt_release", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndBoltRelease", get_LastFP());
		}
		else if (ex)
		{
			PlayHUDMotion				("anm_bolt_pull", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndBoltPull", get_LastFP());
		}
		else
		{
			reload_chamber				();
			m_pInventory->Ruck			(this);
		}
		break;
	}
	case eSubstateReloadChamber:
		PlayHUDMotion					("anm_load_chamber", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndLoadChamber", get_LastFP());
		break;
	case eSubstateReloadBoltLock:
		PlayHUDMotion					("anm_bolt_lock", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndBoltLock", get_LastFP());
		break;
	default:
		PlayHUDMotion					("anm_reload", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndReload", get_LastFP());
		break;
	}
}

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
	if (state != eReload && m_sub_state != eSubstateReloadBolt)
		return							inherited::OnAnimationEnd(state);

	switch (m_sub_state)
	{
	case eSubstateReloadDetach:
		m_magazine_slot->loadingDetach	();
		if (m_magazine_slot->isLoading() || !m_actor)
		{
			m_sub_state					= eSubstateReloadAttach;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadAttach:
		m_magazine_slot->finishLoading	();
		if (!m_actor)
			ReloadMagazine				();
		if (m_locked)
		{
			if (m_mag_attach_bolt_release)
				reload_chamber			();
			else if (!m_actor || m_magazine && !m_magazine->Empty())
			{
				m_sub_state				= eSubstateReloadBolt;
				PlayAnimReload			();
				break;
			}
		}
		SwitchState						(eIdle);
		break;
	case eSubstateReloadBolt:
		reload_chamber					();
		if (state == eHiding)
			switch2_Hiding				();
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadChamber:
		loadChamber						(m_current_ammo);
		break;
	case eSubstateReloadBoltLock:
		reload_chamber					();
		m_locked						= true;
		if (m_lock_state_reload && (m_magazine_slot->isLoading() || !m_actor))
		{
			m_sub_state					= (!m_magazine_slot->addons.empty() || !m_actor) ? eSubstateReloadDetach : eSubstateReloadAttach;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	default:
		ReloadMagazine					();
		SwitchState						(eIdle);
		break;
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	if (isEmptyChamber() && HudAnimationExist("anm_shot_l"))
	{
		PlayHUDMotion					("anm_shot_l", FALSE, GetState());
		m_locked						= true;
	}
	else
		PlayHUDMotion					("anm_shots", FALSE, GetState());
}
