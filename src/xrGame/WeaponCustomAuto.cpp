#include "stdafx.h"
#include "WeaponCustomAuto.h"

CWeaponAutoPistol::CWeaponAutoPistol() : CWeaponMagazined(SOUND_TYPE_WEAPON_PISTOL)
{}

void CWeaponAutoPistol::switch2_Fire()
{
	m_bFireSingleShot = true;
	m_iShotNum = 0;
	m_bStopedAfterQueueFired = false;
}
