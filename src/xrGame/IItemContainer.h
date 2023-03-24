#pragma once
#include "inventory_item_object.h"
#include "InventoryBox.h"

class CContainerObject : public CInventoryItemObject,
	public CInventoryContainer
{
private:
	typedef	CInventoryItemObject					inherited;

public:
							CContainerObject		();
	virtual					~CContainerObject		()								{}
	virtual DLL_Pure*		_construct				();
	
	virtual void			Load					(LPCSTR section);
	virtual	void			OnEvent					(NET_Packet& P, u16 type);

private:
			bool			m_content_volume_scale;
			LPCSTR			m_stock;
			u32				m_stock_count;

public:
			LPCSTR			Stock					()								const	{ return m_stock; }
			u32				StockCount				()								const	{ return m_stock_count; }
			
	virtual	float			GetAmount				()								const	{ return ItemsVolume(); }
	virtual	float			GetFill					()								const	{ return ItemsVolume() / GetCapacity(); }
	virtual	float			GetBar					()								const	{ return fLess(ItemsVolume(), GetCapacity()) ? GetFill() : -1.f; }

	virtual	float			Weight					()								const;
	virtual	float			Volume					()								const;
	virtual	u32				Cost					()								const;
};