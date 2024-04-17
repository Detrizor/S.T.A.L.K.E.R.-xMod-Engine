#pragma once
#include "inventory_item_object.h"

class CAddonOwner;

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
	float								m_slot									= -1.f;
	CAddonOwner*						m_owner									= NULL;

	float								m_length;
	shared_str							m_SlotType;
	Fvector2							m_IconOffset;
	bool								m_low_profile;
	shared_str							m_MotionsSuffix;

public:
	void								setRootBoneID							(u16 bone)				{ m_root_bone_id = bone; }
	void								setSlot									(float val)				{ m_slot = val; }
	void								setPos									(float val)				{ m_slot = floor(m_slot) + val; }
	void								setOwner								(CAddonOwner* ao)		{ m_owner = ao; }

	void								updateLocalTransform					(Fmatrix CR$ parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						MotionSuffix						C$	()		{ return m_MotionsSuffix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	u16									getRootBoneID						C$	()		{ return m_root_bone_id; }
	float								getSlot								C$	()		{ return m_slot; }
	float								getPos								C$	()		{ return m_slot - floor(m_slot); }
	CAddonOwner*						getOwner							C$	()		{ return m_owner; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }
	float								getLength							C$	()		{ return m_length; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ trans);
};
