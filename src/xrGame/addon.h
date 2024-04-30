#pragma once
#include "inventory_item_object.h"

class CAddonSlot;

class CAddon : public CInventoryItemObject
{
public:
	enum eLengthType
	{
		Mount,
		ProfileFwd,
		ProfileBwd
	};

private:
	typedef CInventoryItemObject		inherited;

public:
	void								Load								O$	(LPCSTR section);

private:
	Fmatrix 							m_local_transform						= Fidentity;
	Fmatrix 							m_hud_transform							= Fidentity;
	CAddonSlot*							m_slot									= NULL;
	int									m_slot_idx								= -1;
	int									m_pos									= -1;

	shared_str							m_SlotType;
	Fvector2							m_IconOffset;
	bool								m_low_profile;
	shared_str							m_anm_prefix;
	bool								m_front_positioning;
	
	float								m_mount_length;
	Fvector2							m_profile_length;

public:
	void								setSlot									(CAddonSlot* s)		{ m_slot = s; }
	void								setSlotIdx								(int v)				{ m_slot_idx = v; }
	void								setPos									(int v)				{ m_pos = v; }

	void								updateLocalTransform					(Fmatrix CR$ parent_trans);
	void								updateHudTransform						(Fmatrix CR$ parent_trans);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Fvector2 CR$						IconOffset							C$	()		{ return m_IconOffset; }
	shared_str CR$						anmPrefix							C$	()		{ return m_anm_prefix; }
	Fmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Fmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	CAddonSlot*							getSlot								C$	()		{ return m_slot; }
	int									getSlotIdx							C$	()		{ return m_slot_idx; }
	int									getPos								C$	()		{ return m_pos; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }
	bool								isFrontPositioning					C$	()		{ return m_front_positioning; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ trans);
	int									getLength							C$	(float step, eLengthType type = Mount);
};
