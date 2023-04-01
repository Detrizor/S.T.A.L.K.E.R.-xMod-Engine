#include "stdafx.h"
#include "module.h"
#include "inventory_item.h"
#include "GameObject.h"

CModule::CModule(CGameObject* obj) : pO(obj), O(*obj), pI(smart_cast<CInventoryItem*>(obj))
{
}

void CModule::Transfer(u16 id) const
{
	O.Transfer(id);
}
