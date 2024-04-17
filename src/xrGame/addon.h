#pragma once
#include "inventory_item_object.h"

struct SAddonSlot;

class CAddon : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject		inherited;

public:
	void								Load								O$	(LPCSTR section);

private:
	u16									m_root_bone_id							= 0;
	Fmatrix 							m_local_transform						= Fidentity;
	Fmatrix 							m_hud_transform							= Fidentity;
	SAddonSlot*							m_slot									= NULL;
	int									m_slot_idx								= -1;
	int									m_pos									= -1;

	float								m_length;
	shared_str							m_SlotType;
	Fvector2							m_IconOffset;
	bool								m_low_profile;
	shared_str							m_MotionsSuffix;

public:
	void								setRootBoneID							(u16 bone)			{ m_root_bone_id = bone; }
	void								setSlot									(SAddonSlot* s)		{ m_slot = s; }
	void								setSlotIdx								(int v)				{ m_slot_idx = v; }
	void								setPos									(int v)				{ m_pos = v; }

	void								updateLocalTransform					(Fmatrix CR$ parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						MotionSuffix						C$	()		{ return m_MotionsSuffix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	u16									getRootBoneID						C$	()		{ return m_root_bone_id; }
	SAddonSlot*							getSlot								C$	()		{ return m_slot; }
	int									getSlotIdx							C$	()		{ return m_slot_idx; }
	int									getPos								C$	()		{ return m_pos; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ trans);
	int									getLength							C$	(SAddonSlot CPC slot = NULL);
};
