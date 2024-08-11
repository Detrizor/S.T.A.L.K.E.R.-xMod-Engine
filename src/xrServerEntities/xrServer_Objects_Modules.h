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
	u16									m_version								= 0;

public:
	static xptr<CSE_ALifeModule>&		createModule							(xptr<CSE_ALifeModule>* destination, eAlifeModuleTypes type);

	void								setVersion								(u16 version)		{ m_version = version; }

	void							V$	STATE_Write								(NET_Packet& tNetPacket) = 0;
	void							V$	STATE_Read								(NET_Packet& tNetPacket) = 0;
};

class CSE_ALifeModuleInventoryItem : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mInventoryItem; }

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

private:
	u16									m_slot_idx								= u16_max;
	s16									m_slot_pos								= s16_max;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class MAddon;
};

class CSE_ALifeModuleScope : public CSE_ALifeModule
{
public:
	static eAlifeModuleTypes			mid										()		{ return mScope; }

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

private:
	u8									m_status								= 0;

	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

	friend class MFoldable;
};
