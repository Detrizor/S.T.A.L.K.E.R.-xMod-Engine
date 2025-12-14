#include "stdafx.h"
#include "xrServer_Objects_Modules.h"

#include "xrServer_Objects_ALife.h"

xptr<CSE_ALifeModule>& CSE_ALifeDynamicObject::construct_module(CSE_ALifeModule::EAlifeModuleTypes eType)
{
	switch (eType)
	{
	case CSE_ALifeModule::mInventoryItem: return m_pModules[eType].construct<CSE_ALifeModuleInventoryItem>(m_wVersion);
	case CSE_ALifeModule::mAmountable: return m_pModules[eType].construct<CSE_ALifeModuleAmountable>(m_wVersion);
	case CSE_ALifeModule::mAddon: return m_pModules[eType].construct<CSE_ALifeModuleAddon>(m_wVersion);
	case CSE_ALifeModule::mScope: return m_pModules[eType].construct<CSE_ALifeModuleScope>(m_wVersion);
	case CSE_ALifeModule::mFoldable: return m_pModules[eType].construct<CSE_ALifeModuleFoldable>(m_wVersion);
	default:
		FATAL("wrong alife module type");
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
	tNetPacket.w_s16					(m_slot_idx);
	tNetPacket.w_s16					(m_slot_pos);
}

void CSE_ALifeModuleAddon::STATE_Read(NET_Packet& tNetPacket)
{
	tNetPacket.r_s16					(m_slot_idx);
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
	if (m_nVersion >= 131)
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
