#include "stdafx.h"
#include "item_storage.h"
#include "GameObject.h"
#include "xrMessages.h"
#include "Level.h"
#include "inventory_item.h"

CInventoryStorage::CInventoryStorage()
{
	m_items.clear						();
}

DLL_Pure* CInventoryStorage::_construct()
{
	m_object							= smart_cast<CGameObject*>(this);
	return								m_object;
}

void CInventoryStorage::OnEvent(NET_Packet& P, u16 type)
{
	u16 id								= NO_ID;
	CObject* obj						= NULL;
	bool dont_create_shell				= false;

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
        P.r_u16							(id);
		obj								= Level().Objects.net_Find(id);
		dont_create_shell				= (type == GE_TRADE_SELL) || (!P.r_eof() && P.r_u8());
	}

	OnEventImpl							(type, id, obj, dont_create_shell);
}

void CInventoryStorage::OnEventImpl(u16 type, u16 id, CObject* obj, bool dont_create_shell)
{
	PIItem item							= smart_cast<PIItem>(obj);
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		m_items.push_back				(item);
		obj->H_SetParent				(m_object);
		obj->setVisible					(FALSE);
		obj->setEnabled					(FALSE);
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		m_items.erase					(std::find(m_items.begin(), m_items.end(), item));
		obj->H_SetParent				(NULL, dont_create_shell);
		break;
	}
}
