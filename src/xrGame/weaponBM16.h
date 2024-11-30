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
	CWeaponChamber						m_chamber_second;
	CAddonSlot*							m_loading_slot_second				= nullptr;
	int									m_reloading_chamber					= -1;
	
	int									GetAmmoMagSize						CO$	()		{ return 2; }

	int									GetAmmoElapsed						CO$	();
	bool								has_ammo_to_shoot					CO$	();
	void								prepare_cartridge_to_shoot			O$	();
	void								OnAnimationEnd						O$	(u32 state);
	void								OnHiddenItem						O$	();
	void								StartReload							O$	(EWeaponSubStates substate);

protected:
	void								PlayAnimReload						O$	();

	DECLARE_SCRIPT_REGISTER_FUNCTION

	LPCSTR								anmType		 						CO$	();

	bool								tryTransfer							O$	(MAddon* addon, bool attach);

public:
	bool								checkSecondChamber					C$	(CAddonSlot* slot);
};

add_to_type_list(CWeaponBM16)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBM16)
