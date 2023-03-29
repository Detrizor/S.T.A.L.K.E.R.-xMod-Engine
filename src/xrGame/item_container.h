#pragma once
#include "item_storage.h"

class CInventoryContainer : public CInventoryStorage
{
private:
	typedef	CInventoryStorage inherited;

public:
							CInventoryContainer		();
	virtual					~CInventoryContainer	()										{}

	virtual	void			Load					(LPCSTR section);

private:
			float			m_capacity;
			
			void			OnInventoryAction		(PIItem item, u16 actionType)	const;
			
protected:
	IC		void			SetCapacity				(float val)								{ m_capacity = val; }

	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

public:
	IC const TIItemContainer&Items					()								const	{ return m_items; }
	IC		float			GetCapacity				()								const	{ return m_capacity; }
	IC		bool			Empty					()								const	{ return m_items.empty(); }

			bool			CanTakeItem				(PIItem iitem)					const;
			void			AddAvailableItems		(TIItemContainer& items_container) const;
			float			ItemsWeight				()								const;
			float			ItemsVolume				()								const;
};

#include "inventory_item_object.h"

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
