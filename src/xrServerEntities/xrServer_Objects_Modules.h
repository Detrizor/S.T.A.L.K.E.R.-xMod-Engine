#pragma once
#include "..\xrEngine\device.h"

enum eModuleTypes
{
	mInventoryItem,
	mAmountable,
	mAddon,
	mScope,
	mFoldable,
	mModuleTypesCount
};

class CSE_ALifeModule
{
public:
	S$ xptr<CSE_ALifeModule>			createModule							(u16 type);

public:
	void							V$	STATE_Write								(NET_Packet& tNetPacket) = 0;
	void							V$	STATE_Read								(NET_Packet& tNetPacket) = 0;

	u16								V$	type								C$	() = 0;
};

class CSE_ALifeModuleInventoryItem : public CSE_ALifeModule
{
	u8									m_icon_index							= 0;

protected:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	u16									type								CO$	()		{ return mInventoryItem; }

	friend class CInventoryItem;
};

class CSE_ALifeModuleAmountable : public CSE_ALifeModule
{
	float								m_amount								= 0.f;

protected:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	u16									type								CO$	()		{ return mAmountable; }

	friend class CAmountable;
};

class CSE_ALifeModuleAddon : public CSE_ALifeModule
{
	u16									m_slot_idx								= u16_max;
	s16									m_slot_pos								= s16_max;

protected:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	u16									type								CO$	()		{ return mAddon; }

	friend class CAddon;
};

class CSE_ALifeModuleScope : public CSE_ALifeModule
{
	float								m_magnification							= 0.f;
	u16									m_zeroing								= 0;
	s8									m_selection								= -1;

protected:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	u16									type								CO$	()		{ return mScope; }

	friend class CScope;
};

class CSE_ALifeModuleFoldable : public CSE_ALifeModule
{
	u8									m_status								= 0;

protected:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	u16									type								CO$	()		{ return mFoldable; }

	friend class CFoldable;
};
