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
		if (HudAnimationExist("anm_pull_bolt"))
		{
			if (HudAnimationExist("anm_pull_bolt_dummy") && !m_shot_shell && m_chamber.empty())
				PlayHUDMotion			("anm_pull_bolt_dummy", FALSE, GetState());
			else
				PlayHUDMotion			("anm_pull_bolt", FALSE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndPullBolt", get_LastFP());
		}
		else
		{
			reload_chamber				();
			m_pInventory->Ruck			(this);
		}
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
		if (m_magazine_slot->isLoading())
		{
			m_sub_state					= eSubstateReloadAttach;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadAttach:
		m_magazine_slot->finishLoading	();
		if (isEmptyChamber() && HudAnimationExist("anm_attach_empty"))
			reload_chamber				();
		SwitchState					(eIdle);
		break;
	case eSubstateReloadBolt:
		reload_chamber					();
		SwitchState						(eIdle);
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
	if (m_chamber.capacity() && GetAmmoElapsed() == 1 && HudAnimationExist("anm_shot_l"))
		PlayHUDMotion					("anm_shot_l", FALSE, GetState());
	else
		PlayHUDMotion					("anm_shots", FALSE, GetState());
}
