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
		if (s->getAttachBone() == "grenade")
		{
			m_slot						= s.get();
			break;
		}
	}
}
