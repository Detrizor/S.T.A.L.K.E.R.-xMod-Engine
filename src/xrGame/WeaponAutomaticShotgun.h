#pragma once
#include "WeaponMagazined.h"
#include "script_export_space.h"

class CAddonSlot;

class CWeaponAutomaticShotgun :	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
	virtual void	Load							(LPCSTR section);
	virtual bool	Action							(u16 cmd, u32 flags);

protected:
	void	OnAnimationEnd							(u32 state) override;
	void	PlayAnimReload							() override;
	
	ESoundTypes		m_eSoundAddCartridge			= SOUND_TYPE_WEAPON_RECHARGING;
	ESoundTypes		m_eSoundClose					= SOUND_TYPE_WEAPON_RECHARGING;
	ESoundTypes		m_eSoundOpen					= SOUND_TYPE_WEAPON_RECHARGING;

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	void								drop_loading							(bool destroy);

protected:
	CAddonSlot*							m_loading_slot							= nullptr;

	void								attach_loading							(CAddonSlot* slot);
	void								detach_loading							(CAddonSlot* slot, bool destroy);
	
	void								OnHiddenItem						O$	();

public:
	bool								tryChargeMagazine						(CWeaponAmmo* ammo);
};

add_to_type_list(CWeaponAutomaticShotgun)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAutomaticShotgun)
