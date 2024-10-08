////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects items for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_ItemsH
#define xrServer_Objects_ALife_ItemsH

#include "xrServer_Objects_ALife.h"
#include "PHSynchronize.h"
#include "inventory_space.h"

#include "character_info_defs.h"
#include "infoportiondefs.h"

#pragma warning(push)
#pragma warning(disable:4005)

class CSE_ALifeItemAmmo;

SERVER_ENTITY_DECLARE_BEGIN0(CSE_ALifeInventoryItem)
public:
	enum {
		inventory_item_state_enabled	= u8(1) << 0,
		inventory_item_angular_null		= u8(1) << 1,
		inventory_item_linear_null		= u8(1) << 2,
	};

	union mask_num_items {
		struct {
			u8	num_items : 5;
			u8	mask      : 3;
		};
		u8		common;
	};

public:
	float							m_fCondition;
	float							m_fMass;
	u32								m_dwCost;
	s32								m_iHealthValue;
	s32								m_iFoodValue;
	float							m_fDeteriorationValue;
	CSE_ALifeObject					*m_self;
	u32								m_last_update_time;
	xr_vector<shared_str>			m_upgrades;

public:
									CSE_ALifeInventoryItem	(LPCSTR caSection);
	virtual							~CSE_ALifeInventoryItem	();
	// we need this to prevent virtual inheritance :-(
	virtual CSE_Abstract			*base					() = 0;
	virtual const CSE_Abstract		*base					() const = 0;
	virtual CSE_Abstract			*init					();
	virtual CSE_Abstract			*cast_abstract			() {return 0;};
	virtual CSE_ALifeInventoryItem	*cast_inventory_item	() {return this;};
	virtual	u32						update_rate				() const;
	virtual BOOL					Net_Relevant			();

			bool					has_upgrade				(const shared_str& upgrade_id);
			void					add_upgrade				(const shared_str& upgrade_id);

private:
	bool							prev_freezed;
	bool							freezed;
					u32				m_freeze_time;
	static const	u32				m_freeze_delta_time;
	static const	u32				random_limit;
					CRandom			m_relevent_random;

public:
	// end of the virtual inheritance dependant code

	IC		bool					attached	() const
	{
		return						(base()->ID_Parent < 0xffff);
	}
	virtual bool					bfUseful();

	/////////// network ///////////////
	SPHNetState						State;
	///////////////////////////////////
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeInventoryItem)
#define script_type_list save_type_list(CSE_ALifeInventoryItem)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeItem,CSE_ALifeDynamicObjectVisual,CSE_ALifeInventoryItem)
	bool							m_physics_disabled;

									CSE_ALifeItem	(LPCSTR caSection);
	virtual							~CSE_ALifeItem	();
	virtual CSE_Abstract			*base			();
	virtual const CSE_Abstract		*base			() const;
	virtual CSE_Abstract			*init					();
	virtual CSE_Abstract			*cast_abstract			() {return this;};
	virtual CSE_ALifeInventoryItem	*cast_inventory_item	() {return this;};
	virtual BOOL					Net_Relevant			();
	virtual void					OnEvent					(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender );
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItem)
#define script_type_list save_type_list(CSE_ALifeItem)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemTorch,CSE_ALifeItem)
//�����
	enum EStats{
		eTorchActive				= (1<<0),
		eNightVisionActive			= (1<<1),
		eAttached					= (1<<2)
	};
	bool							m_active;
	bool							m_nightvision_active;
	bool							m_attached;
									CSE_ALifeItemTorch	(LPCSTR caSection);
    virtual							~CSE_ALifeItemTorch	();
	virtual BOOL					Net_Relevant			();

SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemTorch)
#define script_type_list save_type_list(CSE_ALifeItemTorch)

class CSE_ALifeItemAmmo : public CSE_ALifeItem
{
typedef CSE_ALifeItem inherited;

public:
									CSE_ALifeItemAmmo	(LPCSTR caSection);

	CSE_ALifeItemAmmo*				cast_item_ammo		() override { return this; };

public:
	u16								a_elapsed			= 1;
	u16								m_boxSize			= 1;
	u8								m_mag_pos			= u8_max;

public:
	void UPDATE_Read(NET_Packet& P) override;
	void UPDATE_Write(NET_Packet& P) override;
	void STATE_Read(NET_Packet& P, u16 size) override;
	void STATE_Write(NET_Packet& P) override;
	bool can_switch_offline() const override;
	
	static void script_register(lua_State*);
};

add_to_type_list(CSE_ALifeItemAmmo)
#define script_type_list save_type_list(CSE_ALifeItemAmmo)

class CSE_ALifeItemWeapon : public CSE_ALifeItem
{
typedef CSE_ALifeItem inherited;

public:
	u8								wpn_flags			= 0;

	LPCSTR							m_caAmmoSections;
	u32								m_dwAmmoAvailable;
	u32								m_ef_main_weapon_type;
	u32								m_ef_weapon_type;

									CSE_ALifeItemWeapon	(LPCSTR caSection);
	virtual u32						ef_main_weapon_type	() const;
	virtual u32						ef_weapon_type		() const;
	u8								get_slot			();

	virtual BOOL					Net_Relevant		();

	virtual CSE_ALifeItemWeapon		*cast_item_weapon	() {return this;}

public:
	void STATE_Read(NET_Packet& P, u16 size) override;

	static void script_register(lua_State*);
};

add_to_type_list(CSE_ALifeItemWeapon)
#define script_type_list save_type_list(CSE_ALifeItemWeapon)

class CSE_ALifeItemWeaponMagazined : public CSE_ALifeItemWeapon
{
typedef CSE_ALifeItemWeapon inherited;

public:
	static void script_register(lua_State*);
	CSE_ALifeItemWeaponMagazined(LPCSTR caSection) : CSE_ALifeItemWeapon(caSection) {}
	~CSE_ALifeItemWeaponMagazined() override {}
	CSE_ALifeItemWeapon* cast_item_weapon() override {return this;}

private:
	u8 m_u8CurFireMode = 0;
	float m_ads_shift = 0.f;
	bool m_locked = false;
	bool m_cocked = false;

public:
	void STATE_Read(NET_Packet& P, u16 size) override;
	void STATE_Write(NET_Packet& P) override;

	friend class CWeaponMagazined;
};

add_to_type_list(CSE_ALifeItemWeaponMagazined)
#define script_type_list save_type_list(CSE_ALifeItemWeaponMagazined)

class CSE_ALifeItemWeaponMagazinedWGL : public CSE_ALifeItemWeaponMagazined
{
typedef CSE_ALifeItemWeaponMagazined inherited;

public:
	static void script_register(lua_State*);
	CSE_ALifeItemWeaponMagazinedWGL(LPCSTR caSection) : CSE_ALifeItemWeaponMagazined(caSection) {}
	~CSE_ALifeItemWeaponMagazinedWGL() override {}
	CSE_ALifeItemWeapon* cast_item_weapon() override {return this;}

private:
	u8 m_bGrenadeMode = 0;

public:
	void STATE_Read(NET_Packet& P, u16 size) override;
	void STATE_Write(NET_Packet& P) override;

	friend class CWeaponMagazinedWGrenade;
};

add_to_type_list(CSE_ALifeItemWeaponMagazinedWGL)
#define script_type_list save_type_list(CSE_ALifeItemWeaponMagazinedWGL)

class CSE_ALifeItemWeaponAutoShotGun : public CSE_ALifeItemWeaponMagazined
{
typedef CSE_ALifeItemWeaponMagazined inherited;

public:
	static void script_register(lua_State*);
	CSE_ALifeItemWeaponAutoShotGun(LPCSTR caSection) : CSE_ALifeItemWeaponMagazined(caSection) {}
	CSE_ALifeItemWeapon* cast_item_weapon() override {return this;}

public:
	void STATE_Read(NET_Packet& P, u16 size) override;

	friend class CWeaponAutomaticShotgun;
};

add_to_type_list(CSE_ALifeItemWeaponAutoShotGun)
#define script_type_list save_type_list(CSE_ALifeItemWeaponAutoShotGun)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemDetector,CSE_ALifeItem)
	u32								m_ef_detector_type;
									CSE_ALifeItemDetector(LPCSTR caSection);
	virtual							~CSE_ALifeItemDetector();
	virtual u32						ef_detector_type() const;
	virtual CSE_ALifeItemDetector	*cast_item_detector		() {return this;}
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemDetector)
#define script_type_list save_type_list(CSE_ALifeItemDetector)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemArtefact,CSE_ALifeItem)
	float							m_fAnomalyValue;
									CSE_ALifeItemArtefact	(LPCSTR caSection);
	virtual							~CSE_ALifeItemArtefact	();
	virtual BOOL					Net_Relevant			();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemArtefact)
#define script_type_list save_type_list(CSE_ALifeItemArtefact)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemPDA,CSE_ALifeItem)
	u16								m_original_owner;
	shared_str						m_specific_character;
	shared_str						m_info_portion;

									CSE_ALifeItemPDA(LPCSTR caSection);
	virtual							~CSE_ALifeItemPDA();
	virtual CSE_ALifeItemPDA		*cast_item_pda				() {return this;};
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemPDA)
#define script_type_list save_type_list(CSE_ALifeItemPDA)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemDocument,CSE_ALifeItem)
	shared_str							m_wDoc;
									CSE_ALifeItemDocument(LPCSTR caSection);
	virtual							~CSE_ALifeItemDocument();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemDocument)
#define script_type_list save_type_list(CSE_ALifeItemDocument)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemGrenade,CSE_ALifeItem)
	u32								m_ef_weapon_type;
									CSE_ALifeItemGrenade	(LPCSTR caSection);
	virtual							~CSE_ALifeItemGrenade	();
	virtual u32						ef_weapon_type			() const;
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemGrenade)
#define script_type_list save_type_list(CSE_ALifeItemGrenade)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemExplosive,CSE_ALifeItem)
									CSE_ALifeItemExplosive(LPCSTR caSection);
	virtual							~CSE_ALifeItemExplosive();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemExplosive)
#define script_type_list save_type_list(CSE_ALifeItemExplosive)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemBolt,CSE_ALifeItem)
	u32								m_ef_weapon_type;
									CSE_ALifeItemBolt	(LPCSTR caSection);
	virtual							~CSE_ALifeItemBolt	();
	virtual bool					can_save			() const;
	virtual bool					used_ai_locations	() const;
	virtual u32						ef_weapon_type		() const;
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemBolt)
#define script_type_list save_type_list(CSE_ALifeItemBolt)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemCustomOutfit,CSE_ALifeItem)
	u32								m_ef_equipment_type;
									CSE_ALifeItemCustomOutfit	(LPCSTR caSection);
	virtual							~CSE_ALifeItemCustomOutfit	();
	virtual u32						ef_equipment_type			() const;
	virtual BOOL					Net_Relevant				();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemCustomOutfit)
#define script_type_list save_type_list(CSE_ALifeItemCustomOutfit)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeItemHelmet,CSE_ALifeItem)
									CSE_ALifeItemHelmet	(LPCSTR caSection);
	virtual							~CSE_ALifeItemHelmet	();
	virtual BOOL					Net_Relevant			();
SERVER_ENTITY_DECLARE_END
add_to_type_list(CSE_ALifeItemHelmet)
#define script_type_list save_type_list(CSE_ALifeItemHelmet)

#pragma warning(pop)

#endif