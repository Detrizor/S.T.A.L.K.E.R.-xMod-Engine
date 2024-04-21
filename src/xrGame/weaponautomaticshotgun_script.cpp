#include "pch_script.h"
#include "WeaponAutomaticShotgun.h"

using namespace luabind;

#pragma optimize("s",on)
void CWeaponAutomaticShotgun::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponAutomaticShotgun,CGameObject>("CWeaponAutomaticShotgun")
			.def(constructor<>())
	];
}
