#pragma once

#include "module.h"
#include "inventory_space.h"

class MContainer : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mContainer; }

public:
										MContainer								(CGameObject* obj);

private:
	void								sOnChild							O$	(CGameObject* obj, bool take);
	float								sSumItemData						O$	(EItemDataTypes type);
	xoptional<float>					sGetAmount							O$	();
	xoptional<float>					sGetFill							O$	();
	xoptional<float>					sGetBar								O$	();

private:
	TIItemContainer						m_Items;
	float								m_Capacity;
	float								m_Sum[eItemDataTypesSize];
	bool								m_ArtefactIsolation;
	float								m_RadiationProtection;
	bool								m_content_volume_scale;

	float&								Sum										(int type)		{ return m_Sum[type]; }

	void								InvalidateState							();
	float								Get										(EItemDataTypes type);

	void								OnInventoryAction					C$	(PIItem item, bool take);

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

	void								AddAvailableItems					C$	(TIItemContainer& items_container);
	void								ParentCheck							C$	(PIItem item, bool add);
};
