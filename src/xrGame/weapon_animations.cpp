#include "stdafx.h"
#include "WeaponMagazined.h"
#include "addon.h"
#include "magazine.h"

void CWeaponMagazined::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadDetach:
		PlayHUDMotion					(m_magazine->detachAnm().c_str(), TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndDetach", get_LastFP());
		break;
	case eSubstateReloadAttach:
	{
		m_magazine_slot->loadingAttach	();
		auto la							= m_magazine_slot->getLoadingAddon();
		auto mag						= la->O.getModule<MMagazine>();
		PlayHUDMotion					(mag->attachAnm().c_str(), TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndAttach", get_LastFP());
		break;
	}
	case eSubstateReloadBoltPull:
		if (HudAnimationExist("anm_bolt_pull"))
			PlayHUDMotion				("anm_bolt_pull", TRUE, GetState());
		if (m_bolt_pull_anm.loaded())
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
		if (m_locked && !m_lock_state_shooting)
		{
			if (m_mag_attach_bolt_release)
			{
				m_locked				= false;
				m_chamber.load_from_mag	();
			}
			else if (has_mag_with_ammo())
			{
				m_sub_state				= eSubstateReloadBoltRelease;
				PlayAnimReload			();
				break;
			}
		}
		SwitchState						(eIdle);
		break;
	case eSubstateReloadBoltPull:
		m_chamber.reload				(false);
		SwitchState						(eIdle);
		break;
	case eSubstateReloadChamber:
		loadChamber						(m_current_ammo);
		break;
	case eSubstateReloadBoltLock:
		if (!on_bolt_lock())
			SwitchState					(eIdle);
		break;
	case eSubstateReloadBoltRelease:
		m_locked						= false;
		m_chamber.load_from_mag			();
		SwitchState						(eIdle);
		break;
	default:
		ReloadMagazine					();
		SwitchState						(eIdle);
		break;
	}
}

bool CWeaponMagazined::on_bolt_lock()
{
	m_locked							= true;
	m_chamber.unload					(CWeaponChamber::eDrop);
	
	if (m_lock_state_reload && m_magazine_slot && m_magazine_slot->isLoading())
	{
		m_sub_state						= (m_magazine_slot->addons.empty()) ? eSubstateReloadAttach : eSubstateReloadDetach;
		PlayAnimReload					();
		return							true;
	}

	return								false;
}

void CWeaponMagazined::PlayAnimShoot()
{
	PlayHUDMotion						("anm_shoot", FALSE, GetState());
}
