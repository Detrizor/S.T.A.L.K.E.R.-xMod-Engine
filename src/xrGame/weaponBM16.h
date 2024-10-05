#pragma once

#include "WeaponAutomaticShotgun.h"
#include "script_export_space.h"

class CWeaponBM16 :public CWeaponAutomaticShotgun
{
	typedef CWeaponAutomaticShotgun inherited;

public:
										CWeaponBM16								() : m_chamber_second(this) {}

	void								Load								O$	(LPCSTR section);

private:
	CWeaponChamber							m_chamber_second;
	
	bool								is_full_reload						C$	();
	
	int									GetAmmoMagSize						CO$	()		{ return 2; }

	bool								has_mag_with_ammo					CO$	();
	int									GetAmmoElapsed						CO$	();
	void								prepare_cartridge_to_shoot			O$	();

protected:
	void								PlayAnimShoot						O$	();
	void								PlayAnimReload						O$	();

	DECLARE_SCRIPT_REGISTER_FUNCTION

	LPCSTR								anmType		 						CO$	();

	void								ReloadMagazine						O$	();
	bool								tryTransfer							O$	(MAddon* addon, bool attach);
};

add_to_type_list(CWeaponBM16)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBM16)
