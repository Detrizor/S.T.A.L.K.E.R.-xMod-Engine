#pragma once
#include "inventory_item_object.h"

class CAddon : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject		inherited;

public:
										CAddon									();
										
	void								Load								O$	(LPCSTR section);

private:
	shared_str							m_SlotType;
	Fvector2							m_IconOffset;
	Fvector								m_hud_offset[2];

	void								LoadHudOffset							();

public:
	void								Render									(Fmatrix* pos);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	Fvector CPC							HudOffset							C$	()		{ return m_hud_offset; }
};
