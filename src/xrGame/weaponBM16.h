#pragma once

#include "WeaponAutomaticShotgun.h"
#include "script_export_space.h"

class CWeaponBM16 :public CWeaponAutomaticShotgun
{
	typedef CWeaponAutomaticShotgun inherited;

public:
	virtual			~CWeaponBM16					();
	virtual void	Load							(LPCSTR section);

protected:
			bool	SingleCartridgeReload			();

	virtual void	PlayAnimShoot					();
	virtual void	PlayAnimReload					();
	virtual void	PlayReloadSound					();
	virtual void	PlayAnimIdle					();
	virtual void	PlayAnimIdleMoving				();
	virtual void	PlayAnimIdleSprint				();
	virtual void	PlayAnimShow					();
	virtual void	PlayAnimHide					();
	virtual void	PlayAnimBore					();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponBM16)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBM16)
