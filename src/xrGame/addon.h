#pragma once
#include "inventory_item_object.h"

class CAddon : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject		inherited;

public:
	void								Load								O$	(LPCSTR section);

private:
	shared_str							m_SlotType								= 0;
	Fvector2							m_IconOffset							= vZero2;
	shared_str							m_MotionsSuffix							= 0;
	u16									m_root_bone_id							= 0;
	Fmatrix 							m_local_transform						= Fidentity;
	Fmatrix 							m_hud_transform							= Fidentity;
	float								m_slot									= -1.f;

public:
	void								setRootBoneID							(u16 bone)		{ m_root_bone_id = bone; }
	void								setSlot									(float val)		{ m_slot = val; }

	void								updateLocalTransform					(Fmatrix CPC parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						MotionSuffix						C$	()		{ return m_MotionsSuffix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	u16									getRootBoneID						C$	()		{ return m_root_bone_id; }
	float								getSlot								C$	()		{ return m_slot; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix trans);
};
