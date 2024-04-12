#include "stdafx.h"
#include "grenadelauncher.h"

CGrenadeLauncher::CGrenadeLauncher(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_fGrenadeVel						= pSettings->r_float(section, "grenade_vel");
	m_sFlameParticles					= pSettings->r_string(section, "grenade_flame_particles");
	m_sight_offset[0]					= pSettings->r_fvector3(section, "sight_position");
	m_sight_offset[1]					= pSettings->r_fvector3d2r(section, "sight_rotation");
}
