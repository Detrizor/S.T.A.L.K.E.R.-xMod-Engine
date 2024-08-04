#include "stdafx.h"
#include "muzzle.h"
#include "addon.h"
#include "Weapon.h"

MMuzzle::MMuzzle(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_muzzle_point(pSettings->r_fvector3(section, "muzzle_point")),
m_recoil_pattern(CWeapon::readRecoilPattern(section.c_str(), "muzzle_type")),
m_flash_hider(pSettings->r_bool(section, "flash_hider"))
{
}

Fvector MMuzzle::getFirePoint() const
{
	Fvector res							= m_muzzle_point;
	if (auto addon = O.getModule<MAddon>())
		res.add							(static_cast<Fvector>(addon->getLocalTransform().c));
	return								res;
}
