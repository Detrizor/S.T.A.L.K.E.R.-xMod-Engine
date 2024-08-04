#include "stdafx.h"
#include "module.h"
#include "GameObject.h"
#include "inventory_item_object.h"

CModule::CModule(CGameObject* obj) : O(*obj), I(obj->scast<CInventoryItemObject*>())
{
}

void CModule::Transfer(u16 id) const
{
	O.transfer(id);
}
