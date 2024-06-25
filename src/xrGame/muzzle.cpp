#include "stdafx.h"
#include "muzzle.h"
#include "addon.h"
#include "WeaponMagazined.h"

CMuzzleBase::CMuzzleBase(CGameObject* obj, shared_str CR$ section) : CModule(obj),
m_section(section),
m_muzzle_point(pSettings->r_fvector3(section, "muzzle_point"))
{
}

Fvector CMuzzleBase::getFirePoint() const
{
	Fvector								res;
	if (auto addon = cast<CAddon*>())
		res								= static_cast<Fvector>(addon->getLocalTransform().c);
	else if (auto wpn = cast<CWeaponMagazined*>())
		res								= wpn->getFirePoint();

	res.add								(m_muzzle_point);
	return								res;
}
