#pragma once
#include "WeaponMagazined.h"

class CWeaponAutoPistol : public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;

public:
	CWeaponAutoPistol();

protected:
	void switch2_Fire() override;
};
