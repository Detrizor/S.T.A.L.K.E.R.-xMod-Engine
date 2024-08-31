////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.h
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "inventory_space.h"
#include "hit_immunity.h"
#include "attachable_item.h"
#include "xrserver_objects_alife.h"
#include "xrserver_objects_alife_items.h"
#include "script_export_space.h"
#include "module.h"

bool	ItemCategory		(const shared_str& section, LPCSTR cmp);
bool	ItemSubcategory		(const shared_str& section, LPCSTR cmp);
bool	ItemDivision		(const shared_str& section, LPCSTR cmp);

enum EHandDependence{
	hdNone	= 0,
	hd1Hand	= 1,
	hd2Hand	= 2
};

class CSE_Abstract;
class CGameObject;
class CMissile;
class CHudItem;
class CWeaponAmmo;
class CWeapon;
class CPhysicsShellHolder;
class NET_Packet;
class CEatableItem;
struct SPHNetState;
struct net_update_IItem;

class CInventoryOwner;

struct SHit;

class CSE_ALifeInventoryItem;
typedef CSE_ALifeInventoryItem::mask_num_items	mask_inv_num_items;

struct net_update_IItem
{
	u32					dwTimeStamp;
	SPHNetState			State;
};

struct net_updateInvData
{
	xr_deque<net_update_IItem>	NET_IItem;
	u32				m_dwIStartTime;
	u32				m_dwIEndTime;
};

class CInventoryItem : public CAttachableItem,
	public CHitImmunity,
	public CModule
#ifdef DEBUG
	, public pureRender
#endif
{
public:
	static EModuleTypes					mid										()		{ return mInventoryItem; }

private:
	typedef CAttachableItem inherited;

protected:
	enum EIIFlags{				FdropManual			=(1<<0),
								FCanTake			=(1<<1),
								FCanTrade			=(1<<2),
								Fbelt				=(1<<3),
								Fruck				=(1<<4),
								FRuckDefault		=(1<<5),
								FUsingCondition		=(1<<6),
								FAllowSprint		=(1<<7),
								Fuseful_for_NPC		=(1<<8),
								FInInterpolation	=(1<<9),
								FInInterpolate		=(1<<10),
								FIsQuestItem		=(1<<11),
								FIsHelperItem		=(1<<12),
								FCanStack			=(1<<13),
								FShowFullCondition	=(1<<14)
	};

	Flags16						m_flags;
	BOOL						m_can_trade;

public:
								CInventoryItem		(CGameObject* obj);
	virtual						~CInventoryItem		();

public:
	virtual void				Load				(LPCSTR section);
	
	virtual void				OnEvent				(NET_Packet& P, u16 type);
	
	virtual bool				Useful				() const;									// !!! Переопределить. (см. в Inventory.cpp)
	virtual bool				IsUsingCondition	() const { return (m_flags.test(FUsingCondition) > 0); };
	virtual bool				ShowFullCondition	() const { return (m_flags.test(FShowFullCondition) > 0); };
	virtual bool				CanStack			() const;

	virtual EHandDependence		HandDependence		()	const	{return hd1Hand;};
	virtual bool				IsSingleHanded		()	const	{return true;};	
	virtual bool				ActivateItem		();									// !!! Переопределить. (см. в Inventory.cpp)
	virtual void				DeactivateItem		();								// !!! Переопределить. (см. в Inventory.cpp)
	virtual bool				Action				(u16 cmd, u32 flags) {return false;}	// true если известная команда, иначе false
	virtual void				DiscardState		() {};

	virtual void				OnH_B_Chield		();
	virtual void				OnH_A_Chield		();
	virtual void				OnH_B_Independent	(bool just_before_destroy);
	virtual void				OnH_A_Independent	();

	virtual void				save				(NET_Packet &output_packet);
	virtual void				load				(IReader &input_packet);
	virtual BOOL				net_SaveRelevant	()								{return TRUE;}

	virtual void				render_item_ui		()								{}; //when in slot & query return TRUE
	virtual bool				render_item_ui_query()								{return false;}; //when in slot

	virtual void				UpdateCL			();

	virtual	void				Hit					(SHit* pHDS);

			BOOL				GetDropManual		() const	{ return m_flags.test(FdropManual);}
			void				SetDropManual		(BOOL val);

			BOOL				IsInvalid			() const;

			BOOL				IsQuestItem			()	const	{return m_flags.test(FIsQuestItem);}

public:
	CInventory*					m_pInventory;
	shared_str					m_section_id;
	bool						m_highlight_equipped;

	SInvItemPlace				m_ItemCurrPlace;
	SInvItemPlace				m_ItemCurrPlaceBackup;


	virtual void				OnMoveToSlot		(const SInvItemPlace& prev) {};
	virtual void				OnMoveToBelt		(const SInvItemPlace& prev) {};
	virtual void				OnMoveToRuck		(const SInvItemPlace& prev) {};
					
	virtual	Frect				GetIconRect			() const;
			Irect				GetUpgrIconRect		() const;
			const shared_str&	GetIconName			() const		{return m_icon_name;};
			Frect				GetKillMsgRect		() const;
	//---------------------------------------------------------------------
	IC		float				GetCondition		() const					{return m_condition;}
			float				GetConditionToWork	() const;
	virtual	float				GetConditionToShow	() const					{return GetCondition();}
	IC		void				SetCondition		(float val)					{m_condition = val;}
			void				ChangeCondition		(float fDeltaCondition);

			u16					BaseSlot			()  const					{return m_ItemCurrPlace.base_slot_id;}
			u16					HandSlot			()  const					{return m_ItemCurrPlace.hand_slot_id;}
			u16					CurrSlot			()  const					{return (m_ItemCurrPlace.type == eItemPlaceSlot) ? m_ItemCurrPlace.slot_id : NO_ACTIVE_SLOT;}
			u16					CurrPocket			()  const					{return (m_ItemCurrPlace.type == eItemPlacePocket) ? m_ItemCurrPlace.slot_id : 0;}
			u16					CurrPlace			()  const					{return m_ItemCurrPlace.type;}

			bool				InHands				()	const;

			bool				Ruck				()							{return !!m_flags.test(Fruck);}
			void				Ruck				(bool on_ruck)				{m_flags.set(Fruck,on_ruck);}
			bool				RuckDefault			()							{return !!m_flags.test(FRuckDefault);}
			
	virtual bool				CanTake				() const					{return !!m_flags.test(FCanTake);}
			bool				CanTrade			() const;
			void				AllowTrade			()							{ m_flags.set(FCanTrade, m_can_trade); };
			void				DenyTrade			()							{ m_flags.set(FCanTrade, FALSE); };

	virtual bool 				IsNecessaryItem	    (CInventoryItem* item);
	virtual bool				IsNecessaryItem	    (const shared_str& item_sect){return false;};

protected:
	ALife::_TIME_ID				m_dwItemIndependencyTime;

	float						m_fControlInertionFactor;
	shared_str					m_icon_name;

public:
	virtual void				make_Interpolation	()			{};
	virtual void				PH_A_CrPr			(); // actions & operations after phisic correction-prediction steps

	virtual void				net_Import			(NET_Packet& P);					// import from server
	virtual void				net_Export			(NET_Packet& P);					// export to server

public:
	virtual void				activate_physic_shell		();
	virtual bool				has_network_synchronization	() const;

	virtual ALife::_TIME_ID		TimePassedAfterIndependant	() const;

	virtual	bool				IsSprintAllowed				() const		{return !!m_flags.test(FAllowSprint);} ;

	virtual	float				GetControlInertionFactor	(bool full = false) const		{ return m_fControlInertionFactor; }

	virtual void				UpdateXForm	();
			
protected:
	net_updateInvData*				m_net_updateData;
	net_updateInvData*				NetSync						();
	void						CalculateInterpolationParams();

public:
	virtual BOOL				net_Spawn				(CSE_Abstract* DC);
	virtual void				net_Destroy				();
	virtual void				reload					(LPCSTR section);
	virtual void				reinit					();
	virtual bool				can_kill				() const;
	virtual CInventoryItem*		can_kill				(CInventory *inventory) const;
	virtual const CInventoryItem*can_kill				(const xr_vector<const CGameObject*> &items) const;
	virtual CInventoryItem*		can_make_killing		(const CInventory *inventory) const;
	virtual bool				ready_to_kill			() const;
	IC		bool				useful_for_NPC			() const;

public:
	virtual DLL_Pure*			_construct					();
	IC	CPhysicsShellHolder&	object						() const{ VERIFY		(m_object); return		(*m_object);}
	u16							object_id					() const;
	u16							parent_id					() const;
	virtual void				on_activate_physic_shell	() { R_ASSERT2(0, "failed call of virtual function!"); }

public:
	virtual	void				modify_holder_params		(float &range, float &fov) const {}

protected:
	IC	CInventoryOwner&		inventory_owner				() const;

private:
	CPhysicsShellHolder*		m_object;

public:
	virtual CInventoryItem		*cast_inventory_item		()	{return this;}
	virtual CAttachableItem		*cast_attachable_item		()	{return this;}
	virtual CPhysicsShellHolder	*cast_physics_shell_holder	()	{return 0;}
	virtual CEatableItem		*cast_eatable_item			()	{return 0;}
	virtual CWeapon				*cast_weapon				()	{return 0;}
	virtual CMissile			*cast_missile				()	{return 0;}
	virtual CHudItem			*cast_hud_item				()	{return 0;}
	virtual CWeaponAmmo			*cast_weapon_ammo			()	{return 0;}
	virtual CGameObject			*cast_game_object			()  {return 0;}

	////////// upgrades //////////////////////////////////////////////////
public:
	typedef xr_vector<shared_str>	Upgrades_type;

protected:
	Upgrades_type	m_upgrades;

public:
	IC bool	has_any_upgrades			() { return (m_upgrades.size() != 0); }
	bool	has_upgrade					( const shared_str& upgrade_id );
	bool	has_upgrade_group			( const shared_str& upgrade_group_id );
	void	add_upgrade					( const shared_str& upgrade_id, bool loading );
	bool	get_upgrades_str			( string2048& res ) const;
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
	Upgrades_type get_upgrades() { return m_upgrades; }	//Alundaio
#endif

	bool	equal_upgrades				( Upgrades_type const& other_upgrades ) const;

	bool	verify_upgrade				( LPCSTR section );
	bool	install_upgrade				( LPCSTR section );
	void	pre_install_upgrade			();

#ifdef DEBUG	
	void	log_upgrades				();
#endif // DEBUG

	IC Upgrades_type const& upgardes	() const;
	virtual void	Interpolate			();
	float	interpolate_states			(net_update_IItem const & first, net_update_IItem const & last, SPHNetState & current);

protected:
	virtual	void	net_Spawn_install_upgrades	( Upgrades_type saved_upgrades );
	virtual bool	install_upgrade_impl		( LPCSTR section, bool test );

	void								net_Export_PH_Params			(NET_Packet& P, SPHNetState& State, mask_inv_num_items&	num_items);
	void								net_Import_PH_Params			(NET_Packet& P, net_update_IItem& N, mask_inv_num_items& num_items);

	bool								m_just_after_spawn;
	bool								m_activated;

public:
	IC bool	is_helper_item				()				 { return !!m_flags.test(FIsHelperItem); }
	IC void	set_is_helper				(bool is_helper) { m_flags.set(FIsHelperItem,is_helper); }
	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	shared_str							m_category;
	shared_str							m_subcategory;
	shared_str							m_division;
	Frect								m_inv_icon;
	bool								m_inv_icon_types;
	u8									m_inv_icon_type_default;

	u8									m_inv_icon_type							= u8_max;
	u8									m_inv_icon_index						= 0;

public:
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, float& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, int& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, u32& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, u8& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, bool& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, shared_str& value, bool test);
	static	bool			process_if_exists		(LPCSTR section, LPCSTR name, LPCSTR& value, bool test);

			void			SetInvIconType			(u8 type);
			void			SetInvIconIndex			(u8 idx);
			u8				getInvIconType			()								const;
			u8				getInvIconTypeDefault	()								const	{ return m_inv_icon_type; }
			u8				GetInvIconIndex			()								const	{ return m_inv_icon_index; }


	virtual	void			OnTaken					()										{}

private:
	float								m_weight								= 0.f;
	float								m_volume								= 0.f;
	float								m_cost									= 0.f;
	float								m_upgrades_cost							= 0.f;
	float								m_condition								= 1.f;
	shared_str							m_name									= 0;
	shared_str							m_name_short							= 0;
	shared_str							m_description							= 0;
	
	void								set_inv_icon							();

public:
	static const float					s_max_repair_condition;
	static u32							readBaseCost							(LPCSTR section);
	static void							readIcon								(Frect& destination, LPCSTR section, u8 type = 0, u8 idx = 0);
	static LPCSTR						readName								(shared_str CR$ section);
	static LPCSTR						readNameShort							(shared_str CR$ section);

	bool								tryCustomUse							();

	LPCSTR								getName								C$	()		{ return m_name.c_str(); }
	LPCSTR								getNameShort						C$	()		{ return m_name_short.c_str(); }
	LPCSTR								ItemDescription						C$	()		{ return m_description.c_str(); }
	bool								areInvIconTypesAllowed				C$	()		{ return m_inv_icon_types; }

	bool								Category							C$	(LPCSTR cmpc, LPCSTR cmps = "*", LPCSTR cmpd = "*");
	shared_str							Section								C$	(bool full = false);
	float								Price								C$	();

	float								GetAmount							C$	();
	float								GetFill								C$	();
	float								GetBar								C$	();

	float								Weight								C$	();
	float								Volume								C$	();
	float								Cost								C$	();
	
	float								aboba								O$	(EEventTypes type, void* data, int param);
};

#include "inventory_item_inline.h"

add_to_type_list(CInventoryItem)
#undef script_type_list
#define script_type_list save_type_list(CInventoryItem)
