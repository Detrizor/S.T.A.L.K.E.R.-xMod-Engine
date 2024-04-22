#include "stdafx.h"
#include "WeaponMagazined.h"

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
		PlayHUDMotion					("anm_reload_detach", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndReloadDetach", get_LastFP());
		break;
	case eSubstateReloadAttach:
		m_magazine_slot->loadingAttach	();
		PlayHUDMotion					("anm_reload_attach", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndReloadAttach", get_LastFP());
		break;
	case eSubstateReloadBolt:
		if (HudAnimationExist("anm_reload_bolt"))
		{
			PlayHUDMotion				("anm_reload_bolt", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndReloadBolt", get_LastFP());
		}
		else
		{
			reload_chamber				();
			m_pInventory->Ruck			(this);
		}
		break;
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	if (IsZoomed() && HudAnimationExist("anm_shots_aim"))
		PlayHUDMotion					("anm_shots_aim", FALSE, GetState());
	else if (m_chamber.capacity() && GetAmmoElapsed() == 1 && HudAnimationExist("anm_shot_l"))
		PlayHUDMotion					("anm_shot_l", FALSE, GetState());
	else
		PlayHUDMotion					("anm_shots", FALSE, GetState());
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
		if (is_empty())
		{
			if (HudAnimationExist("anm_reload_attach_empty"))
			{
				reload_chamber			();
				SwitchState				(eIdle);
			}
			else
			{
				m_sub_state				= eSubstateReloadBolt;
				PlayAnimReload			();
			}
		}
		else
			SwitchState					(eIdle);
	}
}
