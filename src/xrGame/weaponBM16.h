#pragma once

#include "WeaponAutomaticShotgun.h"
#include "script_export_space.h"

class CWeaponBM16 :public CWeaponAutomaticShotgun
{
	typedef CWeaponAutomaticShotgun inherited;

public:
	void								Load								O$	(LPCSTR section);

protected:
	void								PlayAnimShoot						O$	();
	void								PlayAnimReload						O$	();

	DECLARE_SCRIPT_REGISTER_FUNCTION

	LPCSTR								anmType		 						CO$	();
};

add_to_type_list(CWeaponBM16)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBM16)
