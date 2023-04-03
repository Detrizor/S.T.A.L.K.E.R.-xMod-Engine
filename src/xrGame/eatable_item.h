#pragma once

#include "inventory_item_object.h"
#include "script_export_space.h"

class CEatableItem : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject	inherited;

public:
	virtual CEatableItem*	cast_eatable_item			()	{return this;}

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual void			OnH_A_Independent			();
	virtual	bool			UseBy						(CEntityAlive* npc);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CEatableItem)
#undef script_type_list
#define script_type_list save_type_list(CEatableItem)
