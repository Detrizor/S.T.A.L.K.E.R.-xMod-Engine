#pragma once

#include "inventory_item_object.h"

class CInventoryContainer
{
private:
	CGameObject*			m_object;

public:
							CInventoryContainer		();
	DLL_Pure*				_construct				();

	virtual	void			Load					(LPCSTR section);

private:
	TIItemContainer			m_items;
			float			m_capacity;
			
	void								OnInventoryAction					C$	(PIItem item, bool take);
			
protected:
	IC		void			SetCapacity				(float val)								{ m_capacity = val; }

	void					OnChild			(CObject* obj, bool take);

public:
	IC const TIItemContainer&Items					()								const	{ return m_items; }
	IC		float			GetCapacity				()								const	{ return m_capacity; }
	IC		bool			Empty					()								const	{ return m_items.empty(); }

			bool			CanTakeItem				(PIItem iitem)					const;
			void			AddAvailableItems		(TIItemContainer& items_container) const;
			float			ItemsWeight				()								const;
			float			ItemsVolume				()								const;
};

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

private:
	bool			m_content_volume_scale;
	LPCSTR			m_stock;
	u32				m_stock_count;

public:
	LPCSTR			Stock					()								const	{ return m_stock; }
	u32				StockCount				()								const	{ return m_stock_count; }

	void								OnChild						O$	(CObject* obj, bool take);

	virtual	float			GetAmount				()								const	{ return ItemsVolume(); }
	virtual	float			GetFill					()								const	{ return ItemsVolume() / GetCapacity(); }
	virtual	float			GetBar					()								const	{ return fLess(ItemsVolume(), GetCapacity()) ? GetFill() : -1.f; }

	virtual	float			Weight					()								const;
	virtual	float			Volume					()								const;
	virtual	u32				Cost					()								const;
};
