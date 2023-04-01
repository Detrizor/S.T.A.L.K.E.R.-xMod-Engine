#pragma once

#define CMD_START	(1<<0)
#define CMD_STOP	(1<<1)

enum EInventorySlots
{
	NO_ACTIVE_SLOT	= 0,
	LEFT_HAND_SLOT,
	RIGHT_HAND_SLOT,
	BOTH_HANDS_SLOT,
	KNIFE_SLOT,
	PISTOL_SLOT,
	PRIMARY_SLOT,
	SECONDARY_SLOT,
	OUTFIT_SLOT,
	HELMET_SLOT,
	BACKPACK_SLOT,
	HEADLAMP_SLOT,
	PDA_SLOT,
	TORCH_SLOT,
	GRENADE_SLOT,
	BINOCULAR_SLOT,
	BOLT_SLOT,
	DETECTOR_SLOT,
	LAST_SLOT = DETECTOR_SLOT
};

#define RUCK_HEIGHT			280
#define RUCK_WIDTH			7

class CInventoryItem;
class CInventory;

typedef CInventoryItem*				PIItem;
typedef xr_vector<PIItem>			TIItemContainer;

enum eItemPlace
{			
	eItemPlaceUndefined = 0,
	eItemPlaceSlot,
	eItemPlaceRuck,
	eItemPlacePocket
};

struct SInvItemPlace
{
	union{
		struct{
			u16 type				: 4;
			u16 slot_id				: 6;
			u16 base_slot_id		: 6;
			u16 hand_slot_id		: 6;
		};
		u16	value;
	};
};

extern u16	INV_STATE_LADDER;
extern u16	INV_STATE_CAR;
extern u16	INV_STATE_BLOCK_ALL;
extern u16	INV_STATE_INV_WND;
extern u16	INV_STATE_BUY_MENU;

struct II_BriefInfo
{
	shared_str		icon;
	shared_str		cur_ammo;
	shared_str		fmj_ammo;
	shared_str		ap_ammo;
	shared_str		third_ammo; //Alundaio
	shared_str		fire_mode;

	shared_str		grenade;

	II_BriefInfo() { clear(); }
	
	IC void clear()
	{
		icon		= "";
		cur_ammo	= "";
		fmj_ammo	= "";
		ap_ammo		= "";
		third_ammo = ""; //Alundaio
		fire_mode	= "";
		grenade		= "";
	}
};
