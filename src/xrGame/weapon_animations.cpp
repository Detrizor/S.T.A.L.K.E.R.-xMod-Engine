#include "stdafx.h"
#include "WeaponMagazined.h"

void CWeaponMagazined::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		PlayHUDMotion					("anm_reload", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndReload", get_LastFP());
		break;
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
	}
}

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
	if (state != eReload)
		return							inherited::OnAnimationEnd(state);

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		ReloadMagazine					();
		SwitchState						(eIdle);
		break;
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
		SwitchState					(eIdle);
		break;
	case eSubstateReloadBolt:
		reload_chamber					();
		SwitchState						(eIdle);
		break;
	case eSubstateReloadChamber:
		loadChamber						(m_current_ammo);
		break;
	}
}

void CWeaponMagazined::PlayAnimIdle()
{
	if (ADS())
		PlayHUDMotion					("anm_idle_aim", TRUE, GetState());
	else
		inherited::PlayAnimIdle			();
}

void CWeaponMagazined::PlayAnimShow()
{
	PlayHUDMotion						("anm_show", FALSE, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
	PlayHUDMotion						("anm_hide", TRUE, GetState());
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