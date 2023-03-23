////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "xrmessages.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "UIGameCustom.h"
#include "ui/UIActorMenu.h"

CEatableItem::CEatableItem()
{
	m_physic_item = 0;
	m_iMaxUses = 1;
	m_bRemoveAfterUse = false;
	m_bConditionVolumeScale = false;
	m_CustomData = "";
}

CEatableItem::~CEatableItem()
{
}

DLL_Pure *CEatableItem::_construct	()
{
	m_physic_item	= smart_cast<CPhysicItem*>(this);
	return			(inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
	inherited::Load(section);
	
	m_iMaxUses					= READ_IF_EXISTS(pSettings, r_u8, section, "max_uses", 1);
	m_bRemoveAfterUse			= !!READ_IF_EXISTS(pSettings, r_bool, section, "remove_after_use", false);
	m_fWeightFull				= m_weight;
	m_fWeightEmpty				= READ_IF_EXISTS(pSettings, r_float, section, "empty_weight", (m_bRemoveAfterUse) ? 0.f : m_fWeightFull);
	m_fVolumeFull				= m_volume;
	m_fVolumeEmpty				= READ_IF_EXISTS(pSettings, r_float, section, "empty_volume", (m_bRemoveAfterUse) ? 0.f : m_fVolumeFull);
	m_bConditionVolumeScale		= (m_fVolumeFull != m_fVolumeEmpty);
	m_bDiscreteCondition		= !!READ_IF_EXISTS(pSettings, r_bool, section, "discrete_condition", false);
	m_bDrainOnUse				= !!READ_IF_EXISTS(pSettings, r_bool, section, "drain_on_use", true);
}

void CEatableItem::load( IReader &packet )
{
	inherited::load( packet );
}

void CEatableItem::save( NET_Packet &packet )
{
	inherited::save( packet );
}

BOOL CEatableItem::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC)) return FALSE;
	return TRUE;
};

void CEatableItem::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
}

void CEatableItem::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
}

bool CEatableItem::Useful() const
{
	if(!inherited::Useful()) return false;

	//проверить не все ли еще съедено
	return (!(GetRemainingUses() == 0 && CanDelete()));
}

void CEatableItem::OnH_A_Independent() 
{
	inherited::OnH_A_Independent();
	if(!Useful() && object().Local() && OnServer())
		object().DestroyObject();
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
	if(!Useful()) 
	{
		object().setVisible(FALSE);
		object().setEnabled(FALSE);
		if (m_physic_item)
			m_physic_item->m_ready_to_destroy	= true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}

bool CEatableItem::UseBy(CEntityAlive* entity_alive)
{
	SMedicineInfluenceValues	V;
	V.Load						(m_physic_item->cNameSect());

	CInventoryOwner* IO			= smart_cast<CInventoryOwner*>(entity_alive);
	R_ASSERT					(IO);
	R_ASSERT					(m_pInventory == IO->m_inventory);
	R_ASSERT					(object().H_Parent()->ID() == entity_alive->ID());

	entity_alive->conditions().ApplyInfluence(V, m_physic_item->cNameSect());

	for (u8 i = 0; i < (u8)eBoostMaxCount; i++)
	{
		if (pSettings->line_exist(m_physic_item->cNameSect().c_str(), ef_boosters_section_names[i]))
		{
			SBooster B;
			B.Load(m_physic_item->cNameSect(), (EBoostParams)i);
			entity_alive->conditions().ApplyBooster(B, m_physic_item->cNameSect());
		}
	}

	return true;
}

float CEatableItem::Weight() const
{
	float res = inherited::Weight();
	if (m_iMaxUses > 1)
	{
		float net_weight = m_fWeightFull - m_fWeightEmpty;
		float use_weight = net_weight / m_iMaxUses;
		res = m_fWeightEmpty + (GetRemainingUses() * use_weight);
	}
	return res;
}

float CEatableItem::Volume() const
{
	float res = inherited::Volume();
	if (m_bConditionVolumeScale)
	{
		float net_volume = m_fVolumeFull - m_fVolumeEmpty;
		float use_volume = net_volume / m_iMaxUses;
		res = m_fVolumeEmpty + (GetRemainingUses() * use_volume);
	}
	return res;
}