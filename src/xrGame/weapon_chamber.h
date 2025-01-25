#pragma once

class CWeaponMagazined;
class CCartridge;
class CAddonSlot;
class MAddon;
class CWeaponAmmo;

class CWeaponChamber
{
	CWeaponMagazined&					O;

public:
	enum EUnloadDestination
	{
		eDrop,
		eMagazine,
		eInventory
	};

public:
										CWeaponChamber							(CWeaponMagazined* obj) : O(*obj) {}

private:
	CAddonSlot*							m_slot									= nullptr;
	
	MAddon*								get									C$	();
	void								drop								C$	(MAddon* chamber);
	void								create_shell						C$	(CWeaponAmmo* chamber);


public:
	void								setSlot									(CAddonSlot* slot)		{ m_slot = slot; }
	
										operator bool						C$	()		{ return !!m_slot; }
	bool								loaded								C$	()		{ return !!getAmmo(); }
	CAddonSlot CP$						getSlot								C$	()		{ return m_slot; }
	
	CWeaponAmmo*						getAmmo								C$	();
	bool								empty								C$	();
	void								load								C$	(CCartridge CR$ cartridge);
	void								load								C$	(CWeaponAmmo* ammo);
	void								load_from_mag						C$	();
	void								reload								C$	(bool expended);
	void								unload								C$	(EUnloadDestination destination);
	void								consume								C$	();
};
