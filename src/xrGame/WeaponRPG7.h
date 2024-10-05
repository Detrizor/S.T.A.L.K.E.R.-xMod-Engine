#pragma once

//#include "weaponpistol.h"
#include "weaponcustompistol.h"
#include "rocketlauncher.h"
#include "script_export_space.h"

class CWeaponRPG7 :	public CWeaponCustomPistol,
					public CRocketLauncher
{
private:
	typedef CWeaponCustomPistol inherited;

public:
	virtual BOOL net_Spawn		(CSE_Abstract* DC);
	virtual void OnEvent		(NET_Packet& P, u16 type);
	virtual void ReloadMagazine	();
	virtual void Load			(LPCSTR section);
	virtual void switch2_Fire	();

protected:
	shared_str	m_sRocketSection;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CWeaponRPG7)
#undef script_type_list
#define script_type_list save_type_list(CWeaponRPG7)
