#include "stdafx.h"
#include "xrServer_Objects_Modules.h"

::std::unique_ptr<CSE_ALifeModule> CSE_ALifeModule::createModule(u16 type)
{
	switch (type)
	{
	case mAmountable:
		return							::std::make_unique<CSE_ALifeModuleAmountable>();
	default:
		return							nullptr;
	}
}

void CSE_ALifeModuleAmountable::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_float					(amount);
}

void CSE_ALifeModuleAmountable::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_float					(amount);
}
