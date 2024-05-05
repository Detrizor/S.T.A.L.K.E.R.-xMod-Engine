#include "stdafx.h"
#include "grenadelauncher.h"
#include "addon.h"
#include "WeaponMagazinedWGrenade.h"

CGrenadeLauncher::CGrenadeLauncher(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_fGrenadeVel						= pSettings->r_float(section, "grenade_vel");
	m_sFlameParticles					= pSettings->r_string(section, "grenade_flame_particles");
	m_sight_offset[0]					= pSettings->r_fvector3(section, "sight_position");
	m_sight_offset[1]					= pSettings->r_fvector3d2r(section, "sight_rotation");

	auto ao								= cast<CAddonOwner*>();
	for (auto s : ao->AddonSlots())
	{
		if (s->attach == "grenade")
		{
			m_slot						= s;
			break;
		}
	}
}

float CGrenadeLauncher::aboba(EEventTypes type, void* data, int param)
{
	if (!m_wpn)
		return							CModule::aboba(type, data, param);

	switch (type)
	{
	case eTransferAddon:
	{
		auto addon = (CAddon*)data;
		if (addon->cast<CWeaponAmmo*>())
		{
			m_slot->startReloading		((param) ? (CAddon*)data : nullptr);
			m_wpn->m_sub_state			= (m_slot->addons.empty()) ? CWeapon::eSubstateReloadAttachG : CWeapon::eSubstateReloadDetachG;
			m_wpn->SwitchState			(CWeapon::eReload);
			return						1.f;
		}
		break;
	}
	}

	return								CModule::aboba(type, data, param);
}
