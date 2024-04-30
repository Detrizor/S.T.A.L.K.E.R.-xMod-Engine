#include "stdafx.h"
#include "weaponBM16.h"

void CWeaponBM16::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_sounds.LoadSound					(*HudSection(), "snd_reload_1", "sndReload1", true, m_eSoundReload);
}

void CWeaponBM16::PlayAnimShoot()
{
	PlayHUDMotion						((m_magazin.size()) ? "anm_shot" : "anm_shot_1", TRUE, GetState());
}

void CWeaponBM16::PlayAnimReload()
{
	bool single							= (m_magazin.size() == 1 || !has_ammo_for_reload(2));
	PlayHUDMotion						((single) ? "anm_reload_1" : "anm_reload", TRUE, GetState());
	if (m_sounds_enabled)
		PlaySound						((single) ? "sndReload1" : "sndReload", get_LastFP());
}

LPCSTR CWeaponBM16::anmType() const
{
	switch (m_magazin.size())
	{
	case 0:
		return							"_empty";
	case 1:
		return							"_one";
	default:
		return							"";
	}
}
