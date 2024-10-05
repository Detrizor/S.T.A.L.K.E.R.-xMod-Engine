#include "stdafx.h"
#include "weaponBM16.h"
#include "addon.h"

void CWeaponBM16::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_reload_1", "sndReload1", true, m_eSoundReload);

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
}

bool CWeaponBM16::is_full_reload() const
{
	return								(!m_chamber.loaded() || m_chamber_second.loaded());
}

bool CWeaponBM16::has_mag_with_ammo() const
{
	return								m_chamber.loaded();
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

void CWeaponBM16::prepare_cartridge_to_shoot()
{
	auto& chamber						= (m_chamber_second.loaded()) ? m_chamber_second : m_chamber;
	chamber.consume						();
}

void CWeaponBM16::PlayAnimShoot()
{
	PlayHUDMotion						((m_chamber.loaded()) ? "anm_shoot" : "anm_shoot_1", FALSE, GetState());
}

void CWeaponBM16::PlayAnimReload()
{
	m_sub_state							= eSubstateReloadBegin;
	bool full							= is_full_reload() && has_ammo_for_reload(2);
	PlayHUDMotion						((full) ? "anm_reload" : "anm_reload_1", TRUE, GetState());
	if (m_sounds_enabled)
		PlaySound						((full) ? "sndReload" : "sndReload1", get_LastFP());
}

LPCSTR CWeaponBM16::anmType() const
{
	if (m_chamber.empty())
		return							"_empty";
	if (m_chamber_second.empty())
		return							"_one";
	return								"";
}

void CWeaponBM16::ReloadMagazine()
{
	m_BriefInfo_CalcFrame = 0;
	
	int count							= try_consume_ammo(is_full_reload() ? 2 : 1);
	R_ASSERT							(count);
	if (count == 2 || !m_chamber.loaded())
	{
		m_chamber.load					();
		--count;
	}
	if (count)
		m_chamber_second.load			();
}

bool CWeaponBM16::tryTransfer(MAddon* addon, bool attach)
{
	if (m_grip && attach)
	{
		if (auto ammo = addon->O.getModule<CWeaponAmmo>())
		{
			initReload					(ammo);
			return						true;
		}
	}
	return								inherited::tryTransfer(addon, attach);
}
