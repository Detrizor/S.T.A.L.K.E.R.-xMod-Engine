#pragma once
#include "physic_item.h"
#include "inventory_item.h"
#include "huditem.h"

class CHudItemObject : public CPhysicItem,
	public CHudItem
{
private:
	typedef CPhysicItem					core;
	typedef CHudItem					wrap;

public:
	CHudItem*							cast_hud_item						O$	()		{ return this; }

public:
	void								activate_physic_shell				O$	()		{ wrap::activate_physic_shell(); }
	void								on_activate_physic_shell			O$	()		{ core::activate_physic_shell(); }

public:
	WRAP_CONSTRUCT						()
	WRAP_VIRTUAL_METHOD1				(void, Load, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD0				(void, OnH_A_Chield, , ;)
	WRAP_VIRTUAL_METHOD0				(void, OnH_B_Chield, , ;)
	WRAP_VIRTUAL_METHOD0				(void, OnH_A_Independent, , ;)
	WRAP_VIRTUAL_METHOD1				(void, OnH_B_Independent, , ;, bool)
	WRAP_VIRTUAL_METHOD1				(BOOL, net_Spawn, return, &&, CSE_Abstract*)
	WRAP_VIRTUAL_METHOD0				(void, net_Destroy, , ;)
	WRAP_VIRTUAL_METHOD0				(void, UpdateCL, , ;)
};

class CInventoryItemObject: public CHudItemObject,
	public CInventoryItem
{
private:
	typedef CHudItemObject				core;
	typedef CInventoryItem				wrap;

public:
	CPhysicsShellHolder*				cast_physics_shell_holder			O$	()		{ return this; }
	CInventoryItem*						cast_inventory_item					O$	()		{ return this; }
	CAttachableItem*					cast_attachable_item				O$	()		{ return this; }
	CWeapon*							cast_weapon							O$	()		{ return NULL; }
	CMissile*							cast_missile						O$	()		{ return NULL; }
	CHudItem*							cast_hud_item						O$	()		{ return this; }
	CWeaponAmmo*						cast_weapon_ammo					O$	()		{ return NULL; }
	CGameObject*						cast_game_object					O$	()		{ return this; }

public:
										CInventoryItemObject					() : CInventoryItem(this) {}

protected:
	float								sSumItemData						O$	(EItemDataTypes type)		{ return CInventoryItem::sSumItemData(type); };

protected:
	bool								use_parent_ai_locations				CO$	()
	{
		return CAttachableItem::use_parent_ai_locations() && (Device.dwFrame != dwXF_Frame);
	}

public:
	u32									ef_weapon_type						CO$	()					{ return 0; }
	bool								ActivateItem						O$	(u16 prev_slot)		{ return core::activateItem(prev_slot); }
	void								DeactivateItem						O$	(u16 slot)			{ core::deactivateItem(slot); }
	BOOL								net_SaveRelevant					O$	()					{ return TRUE; }

	bool								Action								O$	(u16 cmd, u32 flags)
	{
		return (wrap::Action(cmd, flags)) ? true : core::Action(cmd, flags);
	}

public:
	void								UpdateXForm							O$	();
	void								renderable_Render					O$	();
	void								shedule_Update						O$	(u32 T);
	
#ifdef DEBUG
	void								PH_Ch_CrPr							O$	()		{ wrap::PH_Ch_CrPr(); }
protected:
	void								OnRender							O$	()		{ wrap::OnRender(); };
#endif

public:
	WRAP_CONSTRUCT						()
	WRAP_VIRTUAL_METHOD1				(void, Load, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD1				(void, Hit, , ;, SHit*)
	WRAP_VIRTUAL_METHOD0				(void, OnH_A_Independent, , ;)
	WRAP_VIRTUAL_METHOD1				(void, OnH_B_Independent, , ;, bool)
	WRAP_VIRTUAL_METHOD0				(void, OnH_A_Chield, , ;)
	WRAP_VIRTUAL_METHOD0				(void, OnH_B_Chield, , ;)
	WRAP_VIRTUAL_METHOD0				(void, UpdateCL, , ;)
	WRAP_VIRTUAL_METHOD2				(void, OnEvent, , ;, NET_Packet&, u16)
	WRAP_VIRTUAL_METHOD0				(void, net_Destroy, , ;)
	WRAP_VIRTUAL_METHOD1				(void, net_Import, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1				(void, net_Export, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1				(void, save, , ;, NET_Packet&)
	WRAP_VIRTUAL_METHOD1				(void, load, , ;, IReader&)
	WRAP_VIRTUAL_METHOD1				(void, reload, , ;, LPCSTR)
	WRAP_VIRTUAL_METHOD0				(void, reinit, , ;)
	WRAP_VIRTUAL_METHOD0				(void, make_Interpolation, , ;)
	WRAP_VIRTUAL_METHOD0				(void, PH_A_CrPr, , ;)
	WRAP_VIRTUAL_METHOD1				(void, OnMoveToRuck, , ;, SInvItemPlace CR$)
	WRAP_VIRTUAL_METHOD2				(void, sSyncData, , ;, CSE_ALifeDynamicObject*, bool)

public:
	BOOL net_Spawn(CSE_Abstract* DC) override;
};
