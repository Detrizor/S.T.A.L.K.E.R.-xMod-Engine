#include "stdafx.h"
#include "xrServer_Objects_Modules.h"

::std::unique_ptr<CSE_ALifeModule> CSE_ALifeModule::createModule(u16 type)
{
	switch (type)
	{
	case mInventoryItem:
		return							::std::make_unique<CSE_ALifeModuleInventoryItem>();
	case mAmountable:
		return							::std::make_unique<CSE_ALifeModuleAmountable>();
	case mAddon:
		return							::std::make_unique<CSE_ALifeModuleAddon>();
	case mScope:
		return							::std::make_unique<CSE_ALifeModuleScope>();
	default:
		return							nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CSE_ALifeModuleInventoryItem::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_u8						(m_icon_index);
}

void CSE_ALifeModuleInventoryItem::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_u8						(m_icon_index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CSE_ALifeModuleAmountable::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_float					(m_amount);
}

void CSE_ALifeModuleAmountable::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_float					(m_amount);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CSE_ALifeModuleAddon::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_u16					(m_slot_idx);
	tNetPacket.w_s16					(m_slot_pos);
}

void CSE_ALifeModuleAddon::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_u16					(m_slot_idx);
	tNetPacket.r_s16					(m_slot_pos);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CSE_ALifeModuleScope::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_float					(m_magnification);
	tNetPacket.w_u16					(m_zeroing);
	tNetPacket.w_s8						(m_selection);
}

void CSE_ALifeModuleScope::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_float					(m_magnification);
	tNetPacket.r_u16					(m_zeroing);
	tNetPacket.r_s8						(m_selection);
}
