#pragma once

#include "rocketlauncher.h"
#include "WeaponAutomaticShotgun.h"
#include "script_export_space.h"

class CWeaponRG6 :  public CRocketLauncher,
					public CWeaponAutomaticShotgun
{
	typedef CRocketLauncher				inheritedRL;
	typedef CWeaponAutomaticShotgun		inheritedSG;
	
public:
	virtual BOOL	net_Spawn				(CSE_Abstract* DC);
	virtual void	Load					(LPCSTR section);
	virtual void	OnEvent					(NET_Packet& P, u16 type);

protected:
	virtual void	FireStart				();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponRG6)
#undef script_type_list
#define script_type_list save_type_list(CWeaponRG6)
