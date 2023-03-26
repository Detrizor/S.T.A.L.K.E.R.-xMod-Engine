////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_object.h
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item object implementation
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "physic_item.h"
#include "inventory_item.h"

class CInventoryItemObjectOld: public CPhysicItem,
	public CInventoryItem
{
private:
	typedef CPhysicItem core;
	typedef CInventoryItem wrap;

public:
	CPhysicsShellHolder*					cast_physics_shell_holder			O$	()		{ return this; }
	CInventoryItem*							cast_inventory_item					O$	()		{ return this; }
	CAttachableItem*						cast_attachable_item				O$	()		{ return this; }
	CWeapon*								cast_weapon							O$	()		{ return NULL; }
	CFoodItem*								cast_food_item						O$	()		{ return NULL; }
	CMissile*								cast_missile						O$	()		{ return NULL; }
	CHudItem*								cast_hud_item						O$	()		{ return NULL; }
	CWeaponAmmo*							cast_weapon_ammo					O$	()		{ return NULL; }
	CGameObject*							cast_game_object					O$	()		{ return this; }

protected:
	bool									use_parent_ai_locations				CO$	()		{ return CAttachableItem::use_parent_ai_locations(); }

public:
	u32										ef_weapon_type						CO$	()		{ return 0; }
	BOOL									net_SaveRelevant					O$	()		{ return TRUE; }
	void									activate_physic_shell				O$	()		{ CInventoryItem::activate_physic_shell(); }
	void									on_activate_physic_shell			O$	()		{ CPhysicItem::activate_physic_shell(); }
	
#ifdef DEBUG
	void									PH_Ch_CrPr							O$	()		{ CInventoryItem::PH_Ch_CrPr(); }
protected:
	void									OnRender							O$	()		{ CInventoryItem::OnRender(); };
#endif

public:
	WRAP_CONSTRUCT							()
	WRAP_VIRTUAL_METHOD1					(void, Load, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD1					(void, Hit, , ;, SHit*)
	WRAP_VIRTUAL_METHOD0					(void, OnH_A_Independent, , ;)
	WRAP_VIRTUAL_METHOD1					(void, OnH_B_Independent, , ;, bool)
	WRAP_VIRTUAL_METHOD0					(void, OnH_A_Chield, , ;)
	WRAP_VIRTUAL_METHOD0					(void, OnH_B_Chield, , ;)
	WRAP_VIRTUAL_METHOD0					(void, UpdateCL, , ;)
	WRAP_VIRTUAL_METHOD2					(void, OnEvent, , ;, NET_Packet&, u16)
	WRAP_VIRTUAL_METHOD1					(BOOL, net_Spawn, return, &&, CSE_Abstract*)
	WRAP_VIRTUAL_METHOD0					(void, net_Destroy, , ;)
	WRAP_VIRTUAL_METHOD1					(void, net_Import, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1					(void, net_Export, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1					(void, save, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1					(void, load, , ;, IReader&)
	WRAP_VIRTUAL_METHOD0					(void, renderable_Render, , ;)
	WRAP_VIRTUAL_METHOD1					(void, reload, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD0					(void, reinit, , ;)
	WRAP_VIRTUAL_METHOD0					(void, make_Interpolation, , ;)
	WRAP_VIRTUAL_METHOD0					(void, PH_B_CrPr, , ;)
	WRAP_VIRTUAL_METHOD0					(void, PH_I_CrPr, , ;)
	WRAP_VIRTUAL_METHOD0					(void, PH_A_CrPr, , ;)
};

#include "huditem.h"

class CHudItemObject : public CInventoryItemObjectOld,
	public CHudItem
{
public:
	CHudItem*								cast_hud_item						O$	()		{ return this; }

protected:
	bool									use_parent_ai_locations				CO$	()
	{
		return CInventoryItemObjectOld::use_parent_ai_locations() && (Device.dwFrame != dwXF_Frame);
	}

public:
	bool									Action								O$	(u16 cmd, u32 flags)
	{
		return (CInventoryItemObjectOld::Action(cmd, flags)) ? true : CHudItem::Action(cmd, flags);
	}
	void									renderable_Render					O$	()		{ CHudItem::renderable_Render(); }
	void									on_renderable_Render				O$	()		{ CInventoryItemObjectOld::renderable_Render(); }
	bool									ActivateItem						O$	()		{ return CHudItem::ActivateItem(); }
	void									DeactivateItem						O$	()		{ CHudItem::DeactivateItem(); }

private:
	typedef CInventoryItemObjectOld			core;
	typedef CHudItem						wrap;

public:
	WRAP_CONSTRUCT							()
	WRAP_VIRTUAL_METHOD1					(void, Load, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD2					(void, OnEvent, , ;, NET_Packet&, u16)
	WRAP_VIRTUAL_METHOD0					(void, OnH_A_Chield, , ;)
	WRAP_VIRTUAL_METHOD0					(void, OnH_B_Chield, , ;)
	WRAP_VIRTUAL_METHOD0					(void, OnH_A_Independent, , ;)
	WRAP_VIRTUAL_METHOD1					(void, OnH_B_Independent, , ;, bool)
	WRAP_VIRTUAL_METHOD1					(BOOL, net_Spawn, return, &&, CSE_Abstract*)
	WRAP_VIRTUAL_METHOD0					(void, net_Destroy, , ;)
	WRAP_VIRTUAL_METHOD0					(void, UpdateCL, , ;)
	WRAP_VIRTUAL_METHOD1					(void, OnMoveToRuck, , ;, SInvItemPlace CR$)
};

class CInventoryItemObject : public CHudItemObject
{
private:
	typedef CHudItemObject inherited;

public:
	void									OnStateSwitch						O$	(u32 S, u32 oldState);
	void									OnAnimationEnd						O$	(u32 state);
	void									UpdateXForm							O$	();
};
