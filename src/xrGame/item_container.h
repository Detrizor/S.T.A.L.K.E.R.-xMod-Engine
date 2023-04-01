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

public:
	IC const TIItemContainer&Items					()								const	{ return m_items; }
	IC		float			GetCapacity				()								const	{ return m_capacity; }
	IC		bool			Empty					()								const	{ return m_items.empty(); }

			bool			CanTakeItem				(PIItem iitem)					const;
			void			AddAvailableItems		(TIItemContainer& items_container) const;

public:
	void								_OnChild								(CObject* obj, bool take);
	float								_Weight								C$	();
	float								_Volume								C$	();
	float								_Cost								C$	();
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

	void								_OnChild							O$	(CObject* obj, bool take)		{ return CInventoryContainer::_OnChild(obj, take); }

	float								_GetAmount							CO$	()		{ return _Volume(); }
	float								_GetFill							CO$	()		{ return _Volume() / GetCapacity(); }
	float								_GetBar								CO$	()		{ return fLess(_Volume(), GetCapacity()) ? _GetFill() : -1.f; }

	float								_Weight								CO$	()		{ return CInventoryContainer::_Weight(); }
	float								_Volume								CO$	()		{ return CInventoryContainer::_Volume(); }
	float								_Cost								CO$	()		{ return CInventoryContainer::_Cost(); }
};
