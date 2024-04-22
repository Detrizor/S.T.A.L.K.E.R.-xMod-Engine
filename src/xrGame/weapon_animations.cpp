#include "stdafx.h"
#include "WeaponMagazined.h"

void CWeaponMagazined::PlayAnimIdle()
{
	if (ADS())
		PlayHUDMotion("anm_idle_aim", TRUE, GetState());
	else
		inherited::PlayAnimIdle();
}

void CWeaponMagazined::PlayAnimShow()
{
	PlayHUDMotion("anm_show", FALSE, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
	PlayHUDMotion("anm_hide", TRUE, GetState());
}

void CWeaponMagazined::PlayAnimReload()
{
	VERIFY								(GetState() == eReload);

	bool detach							= is_detaching();
	if (HudAnimationExist("anm_reload_empty"))
	{
		LPCSTR anm_name				= (m_chamber.size() || detach) ? "anm_reload" : "anm_reload_empty";
		float signal_point			= (m_chamber.size() || detach) ? m_ReloadHalfPoint : m_ReloadEmptyHalfPoint;
		if (detach && m_pMagazineSlot && m_pMagazineSlot->hasLoadingBone())
			signal_point			*= .5f;
		float stop_point			= (detach) ? signal_point : 0.f;
		PlayHUDMotion				(anm_name, TRUE, GetState(), signal_point, stop_point);
	}
	else
	{
		float signal_point			= m_ReloadEmptyHalfPoint;
		if (detach && m_pMagazineSlot && m_pMagazineSlot->hasLoadingBone())
			signal_point			*= .5f;
		float stop_point			= (detach) ? signal_point : ((m_chamber.size()) ? m_ReloadPartialPoint : 0.f);
		PlayHUDMotion				("anm_reload", TRUE, GetState(), signal_point, stop_point);
	}
	
	if (m_sounds_enabled)
	{
		if (bMisfire)
		{
			//TODO: make sure correct sound is loaded in CWeaponMagazined::Load(LPCSTR section)
			if (m_sounds.FindSoundItem("sndReloadMisfire", false))
				PlaySound				("sndReloadMisfire", get_LastFP());
			else
				PlaySound				("sndReload", get_LastFP());
		}
		else
		{
			if (m_chamber.empty() && !is_detaching() && m_sounds.FindSoundItem("sndReloadEmpty", false))
				PlaySound				("sndReloadEmpty", get_LastFP());
			else
				PlaySound				("sndReload", get_LastFP());
		}
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	if (IsZoomed() && HudAnimationExist("anm_shots_aim"))
		PlayHUDMotion("anm_shots_aim", FALSE, GetState());
	else if (m_chamber.capacity() && GetAmmoElapsed() == 1 && HudAnimationExist("anm_shot_l"))
		PlayHUDMotion("anm_shot_l", FALSE, GetState());
	else
		PlayHUDMotion("anm_shots", FALSE, GetState());
}
