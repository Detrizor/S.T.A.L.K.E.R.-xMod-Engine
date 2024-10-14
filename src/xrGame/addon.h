#pragma once
#include "inventory_item_object.h"

class CAddonSlot;
class MAddonOwner;
struct SAction;

typedef xr_vector<xptr<CAddonSlot>> VSlots;

class MAddon : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mAddon; }

	enum eLengthType
	{
		Mount,
		ProfileFwd,
		ProfileBwd
	};

	enum eSlotStatus
	{
		free,
		attached,
		need_to_attach
	};

	static const int					no_idx									= int_max;

public:
										MAddon									(CGameObject* obj, LPCSTR section);

private:
	const shared_str					m_section;

	shared_str							m_SlotType;
	Fvector2							m_icon_origin;
	bool								m_low_profile;
	bool								m_front_positioning;
	float								m_mount_length;
	Fvector2							m_profile_length;
	shared_str							m_attach_action;

	Dmatrix 							m_local_transform						= Didentity;
	Dmatrix 							m_hud_transform							= Didentity;
	Dmatrix 							m_hud_offset							= Didentity;
	CAddonSlot*							m_slot									= nullptr;

	eSlotStatus							m_slot_status							= free;
	int									m_slot_idx								= no_idx;
	int									m_slot_pos								= no_idx;

	SAction*							get_attach_action					C$	();

	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	static void							addAddonModules							(CGameObject& O, shared_str CR$ addon_sect);

	VSlots*								slots									= nullptr;

	void								setSlotStatus							(eSlotStatus v)			{ m_slot_status = v; }
	void								setSlotIdx								(int v)					{ m_slot_idx = v; }
	void								setSlotPos								(int v)					{ m_slot_pos = v; }
	void								setLocalTransform						(Dmatrix CR$ trans)		{ m_local_transform = trans; }
	void								setLowProfile							(bool status)			{ m_low_profile = status; }
	
	void								startAttaching							(CAddonSlot CPC slot);
	void								onAttach								(CAddonSlot* slot);
	void								onDetach								(int transfer = 1);
	
	void								updateHudTransform						(Dmatrix CR$ parent_trans);
	void								updateHudOffset							(Dmatrix CR$ bone_offset, Dmatrix CR$ root_offset);

	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }
	Dmatrix CR$							getLocalTransform					C$	()		{ return m_local_transform; }
	Dmatrix CR$							getHudTransform						C$	()		{ return m_hud_transform; }
	eSlotStatus							getSlotStatus						C$	()		{ return m_slot_status; }
	CAddonSlot*							getSlot								C$	()		{ return m_slot; }
	int									getSlotIdx							C$	()		{ return m_slot_idx; }
	int									getSlotPos							C$	()		{ return m_slot_pos; }
	bool								isLowProfile						C$	()		{ return m_low_profile; }
	bool								isFrontPositioning					C$	()		{ return m_front_positioning; }
	shared_str CR$						section								C$	()		{ return m_section; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	int									getLength							C$	(float step, eLengthType type = Mount);
	Fvector2 							getIconOrigin						C$	(u8 type);
};
