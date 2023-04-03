#include "pch_script.h"
#include "f1.h"
#include "WeaponAmmo.h"
#include "BottleItem.h"
#include "ExplosiveItem.h"
#include "InventoryBox.h"

using namespace ::luabind;

#pragma optimize("s",on)
void CF1::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CF1,CGameObject>("CF1")
			.def(constructor<>()),
		class_<CExplosiveItem,CGameObject>("CExplosiveItem")
			.def(constructor<>())
	];
}
