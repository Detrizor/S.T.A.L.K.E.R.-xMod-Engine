#include "stdafx.h"
#include "grenadelauncher.h"

CGrenadeLauncher::CGrenadeLauncher()
{
	m_fGrenadeVel				= 0.f;
}

void CGrenadeLauncher::Load(LPCSTR section) 
{
	inherited::Load				(section);
	m_fGrenadeVel				= pSettings->r_float(section, "grenade_vel");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGrenadeLauncherObject::Load(LPCSTR section)
{
	inherited::Load				(section);
	CGrenadeLauncher::Load		(section);
}
