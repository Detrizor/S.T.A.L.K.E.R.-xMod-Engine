#pragma once
#include "inventory_item_object.h"

class CAddon : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject		inherited;

public:
										CAddon									();

private:
	shared_str							m_SlotType;

	void								Load								O$	(LPCSTR section);

public:
	void								Render									(Fmatrix* pos);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
};
