#pragma once
#include "weaponcustomauto.h"

class CWeaponPistol : public CWeaponAutoPistol
{
	typedef CWeaponAutoPistol inherited;

protected:
	bool AllowFireWhileWorking() override
	{
		return true;
	}
};
