#include "stdafx.h"
#include "ActorBackpack.h"
#include "Actor.h"
#include "Inventory.h"
#include "InventoryBox.h"

bool CBackpack::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result					= inherited::install_upgrade_impl(section, test);
	CInventoryContainer* cont	= Cast<CInventoryContainer*>();
	if (cont)
	{
		float					tmp;
		bool result2			= process_if_exists(section, "capacity", tmp, test);
		if (result2)
			cont->SetCapacity	(cont->GetCapacity() + tmp);
		result					|= result2;
	}
	return						result;
}
