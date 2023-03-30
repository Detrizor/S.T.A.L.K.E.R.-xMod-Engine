#include "stdafx.h"
#include "module.h"
#include "inventory_item_object.h"

CInventoryItemObject CR$ CModule::Item() const
{
	return *smart_cast<CInventoryItemObject*>(pO);
}
