////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "inventory_item.h"
#include "script_export_space.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem {
private:
	typedef CInventoryItem	inherited;

protected:
			CPhysicItem*	m_physic_item;

public:
							CEatableItem				();
	virtual					~CEatableItem				();
	virtual	DLL_Pure*		_construct					();
	virtual CEatableItem*	cast_eatable_item			()	{return this;}

	virtual void			Load						(LPCSTR section);
	virtual void			load						(IReader &packet);
	virtual void			save						(NET_Packet &packet);

	virtual BOOL			net_Spawn					(CSE_Abstract* DC);
	virtual void			net_Import					(NET_Packet& P);					// import from server
	virtual void			net_Export					(NET_Packet& P);					// export to server

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual void			OnH_A_Independent			();
	virtual	bool			UseBy						(CEntityAlive* npc);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CEatableItem)
#undef script_type_list
#define script_type_list save_type_list(CEatableItem)