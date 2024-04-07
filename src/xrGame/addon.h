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
	shared_str							m_SlotType								= 0;
	Fvector2							m_IconOffset							= vZero2;
	shared_str							m_MotionsSuffix							= 0;

	Fmatrix 							m_local_transform;
	Fmatrix 							m_hud_transform;

public:
	void								updateLocalTransform					(Fmatrix CPC parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	void								RenderHud								();
	void								RenderWorld								(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						MotionSuffix						C$	()		{ return m_MotionsSuffix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
};
