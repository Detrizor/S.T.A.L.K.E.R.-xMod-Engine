#pragma once
#include "inventory_space.h"

class CInventoryStorage
{
private:
	CGameObject*	m_object;

public:
	CInventoryStorage		();
	virtual					~CInventoryStorage		()										{};
	virtual DLL_Pure*		_construct				();

	virtual	void			OnEvent					(NET_Packet& P, u16 type);

private:
	TIItemContainer	m_items;

protected:
	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

	friend	class			CInventoryContainer;
};
