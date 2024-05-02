#pragma once
#include "..\xrEngine\device.h"

enum eModuleTypes
{
	mAmountable,
	mModuleTypesCount,
	mUndefined
};

class CSE_ALifeModule
{
public:
	S$ ::std::unique_ptr<CSE_ALifeModule> createModule							(u16 type);

public:
	u16									mask								C$	()		{ return u16(1) << type(); }

	void							V$	STATE_Write								(NET_Packet& tNetPacket) = 0;
	void							V$	STATE_Read								(NET_Packet& tNetPacket) = 0;

	u16								V$	type								C$	() = 0;
};

class CSE_ALifeModuleAmountable : public CSE_ALifeModule
{
private:
	void								STATE_Write							O$	(NET_Packet& tNetPacket);
	void								STATE_Read							O$	(NET_Packet& tNetPacket);

public:
	float								amount								= 0.f;

	u16									type								CO$	()		{ return mAmountable; }
};
