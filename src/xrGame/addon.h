#pragma once
#include "inventory_item_object.h"

class CAddonSlot;

class CAddon : public CModule
{
public:
	enum eLengthType
	{
		Mount,
		ProfileFwd,
		ProfileBwd
	};

public:
										CAddon									(CGameObject* obj);

private:
	Fmatrix 							m_local_transform						= Fidentity;
	Fmatrix 							m_hud_transform							= Fidentity;
	CAddonSlot*							m_slot									= NULL;
	u16									m_slot_idx								= u16_max;
	s16									m_slot_pos								= s16_max;

	shared_str							m_SlotType;
	Fvector2							m_IconOffset;
	bool								m_low_profile;
	shared_str							m_anm_prefix;
	bool								m_front_positioning;
	
	float								m_mount_length;
	Fvector2							m_profile_length;
	
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	void								setSlot									(CAddonSlot* s)		{ m_slot = s; }
	void								setSlotIdx								(int v)				{ m_slot_idx = (u16)v; }
	void								setSlotPos								(int v)				{ m_slot_pos = (s16)v; }

	void								updateLocalTransform					(Fmatrix CR$ parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						anmPrefix							C$	()		{ return m_anm_prefix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	CAddonSlot*							getSlot								C$	()		{ return m_slot; }
	int									getSlotIdx							C$	()		{ return (int)m_slot_idx; }
	int									getSlotPos							C$	()		{ return (int)m_slot_pos; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }
	bool								isFrontPositioning					C$	()		{ return m_front_positioning; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ trans);
	int									getLength							C$	(float step, eLengthType type = Mount);
};
