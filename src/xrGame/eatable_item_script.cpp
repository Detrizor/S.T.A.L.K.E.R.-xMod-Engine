#include "pch_script.h"
#include "eatable_item.h"
#include "inventory_item.h"

using namespace luabind;

#pragma optimize("s",on)
void CEatableItem::script_register(lua_State *L)
{
	module(L)
		[
			class_<CEatableItem, CInventoryItem>("CEatableItem")
			//.def(constructor<>())
		];
}