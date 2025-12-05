#pragma once

#include "inventory_item_object.h"

class CBackpack : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject _super;

protected:
	bool install_upgrade_impl(LPCSTR section, bool test) override;
};
