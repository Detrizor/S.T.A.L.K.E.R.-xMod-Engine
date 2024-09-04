#include "stdafx.h"
#include "WeaponMagazined.h"
#include "addon.h"
#include "magazine.h"

void CWeaponMagazined::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadDetach:
	{
		if (m_magazine->detachAnm().size())
		{
			PlayHUDMotion				(m_magazine->detachAnm().c_str(), TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndDetach", get_LastFP());
		}
		else
			OnAnimationEnd				(eReload);
		break;
	}
	case eSubstateReloadAttach:
	{
		m_magazine_slot->loadingAttach	();
		auto mag						= m_magazine_slot->getLoadingAddon()->O.getModule<MMagazine>();
		if (mag->attachAnm().size())
		{
			PlayHUDMotion				(mag->attachAnm().c_str(), TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAttach", get_LastFP());
		}
		else
			SwitchState					(eIdle);
		break;
	}
	case eSubstateReloadBoltPull:
	{
		if (HudAnimationExist("anm_bolt_pull"))
			PlayHUDMotion				("anm_bolt_pull", TRUE, GetState());
		if (m_bolt_pull_anm.name.size())
			playBlendAnm				(m_bolt_pull_anm, GetState(), false, (ArmedMode()) ? 1.f / get_wpn_pos_inertion_factor() : 1.f);
		if (m_sounds_enabled)
			PlaySound					("sndBoltPull", get_LastFP());
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
	case eSubstateReloadBoltRelease:
		PlayHUDMotion					("anm_bolt_release", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndBoltRelease", get_LastFP());
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
	if (state != eReload)
	{
		if (state != eFire)
			inherited::OnAnimationEnd	(state);
		return;
	}

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
		if (m_locked && !m_lock_state_shooting)
		{
			if (m_mag_attach_bolt_release)
			{
				m_locked				= false;
				load_chamber			(true);
			}
			else if (!m_actor || m_magazine && !m_magazine->Empty())
			{
				m_sub_state				= eSubstateReloadBoltRelease;
				PlayAnimReload			();
				break;
			}
		}
		SwitchState						(eIdle);
		break;
	case eSubstateReloadBoltPull:
		reload_chamber					();
		SwitchState						(eIdle);
		break;
	case eSubstateReloadChamber:
		loadChamber						(m_current_ammo);
		break;
	case eSubstateReloadBoltLock:
		m_locked						= true;
		unload_chamber					();
		if (m_lock_state_reload && (m_magazine_slot->isLoading() || !m_actor))
		{
			m_sub_state					= (!m_magazine_slot->addons.empty() || !m_actor) ? eSubstateReloadDetach : eSubstateReloadAttach;
			PlayAnimReload				();
		}
		else
			SwitchState					(eIdle);
		break;
	case eSubstateReloadBoltRelease:
		m_locked						= false;
		load_chamber					(true);
		SwitchState						(eIdle);
		break;
	default:
		ReloadMagazine					();
		SwitchState						(eIdle);
		break;
	}
}

void CWeaponMagazined::PlayAnimShoot()
{
	if (isEmptyChamber() && m_bolt_catch)
		m_locked						= true;
	PlayHUDMotion						("anm_shoot", FALSE, GetState());
}
