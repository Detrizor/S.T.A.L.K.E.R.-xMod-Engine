#include "stdafx.h"
#include "item_storage.h"
#include "GameObject.h"
#include "xrMessages.h"
#include "Level.h"
#include "inventory_item.h"

void CItemStorage::OnEvent(NET_Packet& P, u16 type)
{
	u16 id								= NO_ID;
	CObject* obj						= NULL;
	bool dont_create_shell				= false;
	bool take							= false;

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		take							= true;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		P.r_u16							(id);
		obj								= Level().Objects.net_Find(id);
		dont_create_shell				= (type == GE_TRADE_SELL) || (!P.r_eof() && P.r_u8());
	}

	if (obj)
	{
		PIItem item						= smart_cast<PIItem>(obj);
		if (take)
		{
			obj->H_SetParent			(pO);
			obj->setVisible				(FALSE);
			obj->setEnabled				(FALSE);
		}
		else
			obj->H_SetParent			(NULL, dont_create_shell);
	}

	O.OnEventImpl						(type, id, obj, dont_create_shell);
}
