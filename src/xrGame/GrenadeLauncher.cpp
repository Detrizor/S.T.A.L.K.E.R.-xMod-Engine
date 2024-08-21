#include "stdafx.h"
#include "grenadelauncher.h"
#include "addon.h"
#include "WeaponMagazinedWGrenade.h"

MGrenadeLauncher::MGrenadeLauncher(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_fGrenadeVel						= pSettings->r_float(section, "grenade_vel");
	m_sFlameParticles					= pSettings->r_string(section, "grenade_flame_particles");
	m_sight_offset[0]					= static_cast<Dvector>(pSettings->r_fvector3(section, "sight_position"));
	m_sight_offset[1]					= static_cast<Dvector>(pSettings->r_fvector3d2r(section, "sight_rotation"));

	auto ao								= O.getModule<MAddonOwner>();
	for (auto& s : ao->AddonSlots())
	{
		if (s->attach == "grenade")
		{
			m_slot						= s.get();
			break;
		}
	}
}

float MGrenadeLauncher::aboba(EEventTypes type, void* data, int param)
{
	if (!m_wpn)
		return							CModule::aboba(type, data, param);

	switch (type)
	{
	case eTransferAddon:
	{
		auto addon						= static_cast<MAddon*>(data);
		if (addon->O.scast<CWeaponAmmo*>())
		{
			m_slot->startReloading		((param) ? addon : nullptr);
			m_wpn->m_sub_state			= (m_slot->addons.empty()) ? CWeapon::eSubstateReloadAttachG : CWeapon::eSubstateReloadDetachG;
			m_wpn->SwitchState			(CWeapon::eReload);
			return						1.f;
		}
		break;
	}
	}

	return								CModule::aboba(type, data, param);
}
