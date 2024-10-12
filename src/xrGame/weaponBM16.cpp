#include "stdafx.h"
#include "weaponBM16.h"
#include "Missile.h"
#include "addon.h"

void CWeaponBM16::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_open", "sndOpen", true, m_eSoundReload);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_close", "sndClose", true, m_eSoundReload);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_attach_both", "sndAttachBoth", true, m_eSoundReload);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_detach_both", "sndDetachBoth", true, m_eSoundReload);

	auto ao								= getModule<MAddonOwner>();
	for (auto& s : ao->AddonSlots())
	{
		if (s->getAttachBone() == "chamber_second")
		{
			m_chamber_second.setSlot	(s.get());
			break;
		}
	}
	R_ASSERT							(m_chamber_second);
	
	m_loading_slot						= ao->emplaceSlot();
	m_loading_slot->setAttachBone		("loading");
	m_loading_slot->model_offset.c		= pSettings->r_dvector3(section, "loading_point");
	m_loading_slot_second				= ao->emplaceSlot();
	m_loading_slot_second->setAttachBone("loading_second");
	m_loading_slot_second->model_offset.c = pSettings->r_dvector3(section, "loading_point_second");
}

int CWeaponBM16::GetAmmoElapsed() const
{
	int res								= 0;
	if (m_chamber.loaded())
		++res;
	if (m_chamber_second.loaded())
		++res;
	return								res;
}

bool CWeaponBM16::has_ammo_to_shoot() const
{
	return								(m_chamber.loaded() || m_chamber_second.loaded());
}

void CWeaponBM16::prepare_cartridge_to_shoot()
{
	auto& chamber						= (m_chamber_second.loaded()) ? m_chamber_second : m_chamber;
	chamber.consume						();
}

void CWeaponBM16::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		PlayHUDMotion					("anm_open", TRUE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndOpen", get_LastFP());
		break;
	case eSubstateReloadAttach:
		switch (m_reloading_chamber)
		{
		case 0:
			attach_loading				(m_loading_slot);
			PlayHUDMotion				("anm_attach", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAttach", get_LastFP());
			break;
		case 1:
			attach_loading				(m_loading_slot_second);
			PlayHUDMotion				("anm_attach_second", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAttach", get_LastFP());
			break;
		case 2:
			attach_loading				(m_loading_slot);
			attach_loading				(m_loading_slot_second);
			PlayHUDMotion				("anm_attach_both", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndAttachBoth", get_LastFP());
			break;
		}
		break;
	case eSubstateReloadDetach:
		switch (m_reloading_chamber)
		{
		case 0:
			PlayHUDMotion				("anm_detach", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndDetach", get_LastFP());
			break;
		case 1:
			PlayHUDMotion				("anm_detach_second", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndDetach", get_LastFP());
			break;
		case 2:
			PlayHUDMotion				("anm_detach_both", TRUE, GetState());
			if (m_sounds_enabled)
				PlaySound				("sndDetachBoth", get_LastFP());
			break;
		}
		break;
	case eSubstateReloadEnd:
		if (!m_loading_slot->empty())
			m_chamber.load_from			(m_loading_slot->addons.front()->O.scast<CWeaponAmmo*>());
		if (!m_loading_slot_second->empty())
			m_chamber_second.load_from	(m_loading_slot_second->addons.front()->O.scast<CWeaponAmmo*>());
		m_reloading_chamber				= -1;
		m_current_ammo					= nullptr;
		PlayHUDMotion					("anm_close", TRUE, GetState());
		if (m_sounds_enabled)
				PlaySound				("sndClose", get_LastFP());
		break;
	default:
		inherited::PlayAnimReload		();
	}
}

void CWeaponBM16::OnAnimationEnd(u32 state)
{
	if (state != eReload)
		return							inherited::OnAnimationEnd(state);
	
	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		if (m_reloading_chamber == -1)
			m_sub_state					= eSubstateReloadEnd;
		else
		{
			bool						empty;
			if (m_reloading_chamber == 2)
				empty					= (m_chamber.empty() && m_chamber_second.empty());
			else
			{
				auto& chamber			= (m_reloading_chamber) ? m_chamber_second : m_chamber;
				empty					= chamber.empty();
			}
			m_sub_state					= (empty) ? eSubstateReloadAttach : eSubstateReloadDetach;
		}
		PlayAnimReload					();
		break;
	case eSubstateReloadAttach:
	{
		m_sub_state						= eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	}
	case eSubstateReloadDetach:
		if (m_reloading_chamber == 0 || m_reloading_chamber == 2)
			m_chamber.unload			(CWeaponChamber::eInventory);
		if (m_reloading_chamber == 1 || m_reloading_chamber == 2)
			m_chamber_second.unload		(CWeaponChamber::eInventory);
		m_sub_state						= (m_current_ammo) ? eSubstateReloadAttach : eSubstateReloadEnd;
		PlayAnimReload					();
		break;
	case eSubstateReloadEnd:
		SwitchState						(eIdle);
		break;
	default:
		inherited::OnAnimationEnd		(state);
	}
}

void CWeaponBM16::OnHiddenItem()
{
	if (!m_loading_slot_second->empty())
	{
		auto loading					= m_loading_slot_second->addons.front();
		m_loading_slot_second->detachAddon(loading, true);
	}
	m_reloading_chamber					= -1;
	inherited::OnHiddenItem				();
}

void CWeaponBM16::StartReload(EWeaponSubStates substate)
{
	if (GetState() == eReload)
		return;

	if (substate != eSubstateReloadBegin)
	{
		substate						= eSubstateReloadBegin;
		if (m_actor && m_actor->unlimited_ammo())
		{
			m_chamber.load				();
			m_chamber_second.load		();
		}
	}
	else if (m_reloading_chamber == -1)
		m_reloading_chamber				= 2;
	
	m_sub_state							= substate;
	SwitchState							(eReload);
}

LPCSTR CWeaponBM16::anmType() const
{
	return								"";
}

bool CWeaponBM16::tryTransfer(MAddon* addon, bool attach)
{
	if (m_grip)
	{
		auto ammo						= addon->O.getModule<CWeaponAmmo>();
		auto shell						= addon->O.scast<CMissile*>();
		if (ammo || shell)
		{
			m_current_ammo				= (attach) ? ammo : nullptr;
			if (addon->getSlotIdx() != MAddon::no_idx)
				m_reloading_chamber		= (addon->getSlotIdx() == m_chamber_second.getSlot()->idx) ? 1 : 0;
			
			StartReload					(eSubstateReloadBegin);
			return						true;
		}
	}
	return								inherited::tryTransfer(addon, attach);
}

bool CWeaponBM16::checkSecondChamber(CAddonSlot* slot) const
{
	return								(m_chamber_second.getSlot() == slot);
}
