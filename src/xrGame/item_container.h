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
	float								m_capacity								= 0.f;
	bool								m_artefact_isolation					= false;
	bool								m_content_volume_scale					= false;
	
	TIItemContainer						m_items									= {};
	xoptional<float>					m_sum[eItemDataTypesSize]				= {};

	void								InvalidateState							();
	float								Get										(EItemDataTypes type);

public:
	float								GetWeight								()		{ return Get(eWeight); }
	float								GetVolume								()		{ return Get(eVolume); }

	bool								CanTakeItem								(PIItem iitem);

	void								SetCapacity								(float val)		{ m_capacity = val; }

	bool								ArtefactIsolation					C$	(bool own = false);

	TIItemContainer CR$					Items								C$	()		{ return m_items; }
	float								GetCapacity							C$	()		{ return m_capacity; }
	bool								Empty								C$	()		{ return m_items.empty(); }

	void								AddAvailableItems					C$	(TIItemContainer& items_container);
	void								ParentCheck							C$	(PIItem item, bool add);
};
