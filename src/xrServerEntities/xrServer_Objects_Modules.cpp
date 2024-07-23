#include "stdafx.h"
#include "xrServer_Objects_Modules.h"

xptr<CSE_ALifeModule> CSE_ALifeModule::createModule(u16 type, u16 version)
{
	CSE_ALifeModule* res				= nullptr;
	switch (type)
	{
	case mInventoryItem:
		res								= xr_new<CSE_ALifeModuleInventoryItem>();
		break;
	case mAmountable:
		res								= xr_new<CSE_ALifeModuleAmountable>();
		break;
	case mAddon:
		res								= xr_new<CSE_ALifeModuleAddon>();
		break;
	case mScope:
		res								= xr_new<CSE_ALifeModuleScope>();
		break;
	case mFoldable:
		res								= xr_new<CSE_ALifeModuleFoldable>();
		break;
	}

	if (res)
		res->m_version					= version;

	return								res;
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
