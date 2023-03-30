#include "stdafx.h"
#include "grenadelauncher.h"

CGrenadeLauncher::CGrenadeLauncher(CGameObject* obj) : CModule(obj)
{
	m_fGrenadeVel						= 0.f;
}

void CGrenadeLauncher::Load(LPCSTR section) 
{
	m_fGrenadeVel						= pSettings->r_float(section, "grenade_vel");
}
