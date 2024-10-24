#pragma once
#include "..\xrEngine\device.h"

class CSE_ALifeModule
{
public:
	enum eAlifeModuleTypes
	{
		mModuleTypesBegin,
		mInventoryItem = mModuleTypesBegin,
		mAmountable,
		mAddon,
		mScope,
		mFoldable,
		mModuleTypesEnd
	};

protected:
										CSE_ALifeModule							(u16 version) : m_version(version) {}

protected:
	const u16							m_version;

public:
	static CSE_ALifeModule*				createModule							(eAlifeModuleTypes type, u16 version);

	void							V$	STATE_Write								(NET_Packet& tNetPacket) = 0;
	void							V$	STATE_Read								(NET_Packet& tNetPacket) = 0;
};

class CSE_ALifeModuleInventoryItem : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mInventoryItem; }

public:
										CSE_ALifeModuleInventoryItem			(u16 version) : CSE_ALifeModule(version) {}

private:
	u8									m_icon_index							= 0;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class CInventoryItem;
};

class CSE_ALifeModuleAmountable : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mAmountable; }

public:
										CSE_ALifeModuleAmountable				(u16 version) : CSE_ALifeModule(version) {}

private:
	float								m_amount								= 0.f;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class MAmountable;
};

class CSE_ALifeModuleAddon : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mAddon; }

public:
										CSE_ALifeModuleAddon					(u16 version) : CSE_ALifeModule(version) {}

private:
	s16									m_slot_idx								= s16_max;
	s16									m_slot_pos								= s16_max;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	void								setSlotIdx								(s16 idx)		{ m_slot_idx = idx; }

	friend class MAddon;
};

class CSE_ALifeModuleScope : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mScope; }

public:
										CSE_ALifeModuleScope					(u16 version) : CSE_ALifeModule(version) {}

private:
	float								m_magnification							= 0.f;
	u16									m_zeroing								= 0;
	s8									m_selection								= -1;
	u8									m_current_reticle						= 0;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class MScope;
};

class CSE_ALifeModuleFoldable : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mFoldable; }

public:
										CSE_ALifeModuleFoldable					(u16 version) : CSE_ALifeModule(version) {}

private:
	u8									m_status								= 0;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class MFoldable;
};
