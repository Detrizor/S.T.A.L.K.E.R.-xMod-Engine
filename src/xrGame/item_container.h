#pragma once

#include "module.h"
#include "inventory_space.h"

class CGameObject;
class CInventoryItem;

class CInventoryContainer : public CModule
{
public:
										CInventoryContainer						(CGameObject* obj);

private:
	TIItemContainer						m_Items;
	float								m_Capacity;
	float								m_Sum[3];
	bool								m_ArtefactIsolation;
	float								m_RadiationProtection;

	bool								m_content_volume_scale;
	LPCSTR								m_stock;
	u32									m_stock_count;

	float&								Sum										(int type)		{ return m_Sum[type-eWeight]; }

	void								InvalidateState							();
	float								Get										(EEventTypes type);

	void								OnInventoryAction					C$	(PIItem item, bool take);

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	float								GetWeight								()		{ return Get(eWeight); }
	float								GetVolume								()		{ return Get(eVolume); }

	bool								CanTakeItem								(PIItem iitem);

	void								SetCapacity								(float val)		{ m_Capacity = val; }

	bool								ArtefactIsolation					C$	(bool own = false);
	float								RadiationProtection					C$	(bool own = false);

	TIItemContainer CR$					Items								C$	()		{ return m_Items; }
	float								GetCapacity							C$	()		{ return m_Capacity; }
	bool								Empty								C$	()		{ return m_Items.empty(); }
	LPCSTR								Stock								C$	()		{ return m_stock; }
	u32									StockCount							C$	()		{ return m_stock_count; }

	void								AddAvailableItems					C$	(TIItemContainer& items_container);
	void								ParentCheck							C$	(PIItem item, bool add);
};
