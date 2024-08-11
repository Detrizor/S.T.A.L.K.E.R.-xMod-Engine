#include "stdafx.h"
#include "xrServer_Objects_Modules.h"

xptr<CSE_ALifeModule>& CSE_ALifeModule::createModule(xptr<CSE_ALifeModule>* destination, eAlifeModuleTypes type)
{
	switch (type)
	{
	case mInventoryItem:
		return							destination[type].construct<CSE_ALifeModuleInventoryItem>();
	case mAmountable:
		return							destination[type].construct<CSE_ALifeModuleAmountable>();
	case mAddon:
		return							destination[type].construct<CSE_ALifeModuleAddon>();
	case mScope:
		return							destination[type].construct<CSE_ALifeModuleScope>();
	case mFoldable:
		return							destination[type].construct<CSE_ALifeModuleFoldable>();
	default:
		FATAL							("wrong alife module type");
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
	tNetPacket.w_u8						(m_current_reticle);
}

void CSE_ALifeModuleScope::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_float					(m_magnification);
	tNetPacket.r_u16					(m_zeroing);
	tNetPacket.r_s8						(m_selection);
	if (m_version >= 131)
		tNetPacket.r_u8					(m_current_reticle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void CSE_ALifeModuleFoldable::STATE_Write(NET_Packet& tNetPacket)
{
	tNetPacket.w_u8						(m_status);
}

void CSE_ALifeModuleFoldable::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_u8						(m_status);
}
