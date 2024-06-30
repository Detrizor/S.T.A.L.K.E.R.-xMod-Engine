#pragma once
#include "inventory_item_object.h"

class CAddonSlot;
class CAddonOwner;

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
	Dmatrix 							m_local_transform						= Didentity;
	Dmatrix 							m_hud_transform							= Didentity;
	CAddonSlot*							m_slot									= NULL;
	u16									m_slot_idx								= u16_max;
	s16									m_slot_pos								= s16_max;

	shared_str							m_SlotType;
	Fvector2							m_icon_origin;
	bool								m_low_profile;
	bool								m_front_positioning;
	
	float								m_mount_length;
	Fvector2							m_profile_length;
	
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	static void							addAddonModules							(CGameObject& O, shared_str CR$ addon_sect);

	void								setSlot									(CAddonSlot* s)			{ m_slot = s; }
	void								setSlotIdx								(int v)					{ m_slot_idx = (u16)v; }
	void								setSlotPos								(int v)					{ m_slot_pos = (s16)v; }
	void								setLocalTransform						(Dmatrix CR$ trans)		{ m_local_transform = trans; }

	void								updateHudTransform						(Dmatrix CR$ parent_trans);
	
	void								attach									(CAddonOwner CPC ao, u16 slot_idx);
	bool								tryAttach								(CAddonOwner CPC ao, u16 slot_idx = u16_max);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Dmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Dmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	CAddonSlot*							getSlot								C$	()		{ return m_slot; }
	int									getSlotIdx							C$	()		{ return (int)m_slot_idx; }
	int									getSlotPos							C$	()		{ return (int)m_slot_pos; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }
	bool								isFrontPositioning					C$	()		{ return m_front_positioning; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	int									getLength							C$	(float step, eLengthType type = Mount);
	Fvector2 							getIconOrigin						C$	(u8 type);
};
