#include "stdafx.h"
#include "weaponBM16.h"

void CWeaponBM16::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_sounds.LoadSound					(HudSection().c_str(), "snd_reload_1", "sndReload1", true, m_eSoundReload);
	m_chamber.reserve					(2);
}

void CWeaponBM16::PlayAnimShoot()
{
	PlayHUDMotion						((m_chamber.size()) ? "anm_shoot" : "anm_shoot_1", FALSE, GetState());
}

void CWeaponBM16::PlayAnimReload()
{
	bool single							= (m_chamber.size() == 1 || !has_ammo_for_reload(2));
	PlayHUDMotion						((single) ? "anm_reload_1" : "anm_reload", TRUE, GetState());
	if (m_sounds_enabled)
		PlaySound						((single) ? "sndReload1" : "sndReload", get_LastFP());
}

LPCSTR CWeaponBM16::anmType() const
{
	switch (m_chamber.size())
	{
	case 0:
		return							"_empty";
	case 1:
		return							"_one";
	default:
		return							"";
	}
}

void CWeaponBM16::ReloadMagazine()
{
	m_BriefInfo_CalcFrame				= 0;
	
	int count							= try_consume_ammo(m_chamber.capacity() - m_chamber.size());
	while (count--)
		m_chamber.push_back				(m_cartridge);
}

bool CWeaponBM16::canTake(CWeaponAmmo CPC ammo, bool chamber) const
{
	for (auto& t : m_ammoTypes)
		if (t == ammo->Section())
			return						true;
	return								false;
}
