////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_trader_abstract.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife trader abstract class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_simulator.h"
#include "specific_character.h"
#include "ai_space.h"
#include "alife_object_registry.h"
#include "ai_debug.h"
#include "alife_graph_registry.h"
#include "xrServer.h"
#include "alife_schedule_registry.h"

#ifdef DEBUG
	extern Flags32 psAI_Flags;
#endif

void CSE_ALifeTraderAbstract::spawn_supplies	()
{
	CSE_ALifeDynamicObject		*dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(this);
	VERIFY						(dynamic_object);
	CSE_Abstract				*abstract = dynamic_object->alife().spawn_item("device_pda",base()->o_Position,dynamic_object->m_tNodeID,dynamic_object->m_tGraphID,base()->ID);
	CSE_ALifeItemPDA			*pda = smart_cast<CSE_ALifeItemPDA*>(abstract);
	pda->m_original_owner		= base()->ID;

#ifdef XRGAME_EXPORTS
	character_profile			();
	m_SpecificCharacter			= shared_str();
	m_community_index			= NO_COMMUNITY_INDEX;
	pda->m_specific_character	= specific_character();
#endif

	if(m_SpecificCharacter.size())
	{
		//если в custom data объекта есть
		//секция [dont_spawn_character_supplies]
		//то не вызывать spawn из selected_char.SupplySpawn()
		bool specific_character_supply = true;	

		if (xr_strlen(dynamic_object->m_ini_string))
		{
#pragma warning(push)
#pragma warning(disable:4238)
			CInifile					ini(
				&IReader				(
					(void*)(*dynamic_object->m_ini_string),
					xr_strlen(dynamic_object->m_ini_string)
				),
				FS.get_path("$game_config$")->m_Path
			);
#pragma warning(pop)

			if (ini.section_exist("dont_spawn_character_supplies")) 
				specific_character_supply = false;
		}

		if(specific_character_supply)
		{
			CSpecificCharacter selected_char;
			selected_char.Load(m_SpecificCharacter);
			dynamic_object->spawn_supplies(selected_char.SupplySpawn());
		}
	}
}

void CSE_ALifeTraderAbstract::vfInitInventory()
{
//	m_fCumulativeItemMass		= 0.f;
//	m_iCumulativeItemVolume		= 0;
}

#if 0//def DEBUG
bool CSE_ALifeTraderAbstract::check_inventory_consistency	()
{
	int							volume = 0;
	float						mass = 0.f;
	xr_vector<ALife::_OBJECT_ID>::const_iterator	I = base()->children.begin();
	xr_vector<ALife::_OBJECT_ID>::const_iterator	E = base()->children.end();
	for ( ; I != E; ++I) {
		CSE_ALifeDynamicObject	*object = ai().alife().objects().object(*I,true);
		if (!object)
			continue;

		CSE_ALifeInventoryItem	*item = smart_cast<CSE_ALifeInventoryItem*>(object);
		if (!item)
			continue;

		volume					+= item->m_iVolume;
		mass					+= item->m_fMass;
	}

	R_ASSERT2					(fis_zero(m_fCumulativeItemMass - mass,EPS_L),base()->name_replace());
	if (!fis_zero(m_fCumulativeItemMass - mass,EPS_L))
		return					(false);

	R_ASSERT2					(m_iCumulativeItemVolume == volume,base()->name_replace());
	if (m_iCumulativeItemVolume != volume)
		return					(false);

#ifdef DEBUG
//	if (psAI_Flags.test(aiALife)) {
//		Msg						("[LSS] [%s] inventory is consistent [%f][%d]",base()->name_replace(),mass,volume);
//	}
#endif

	return						(true);
}
#endif

void CSE_ALifeDynamicObject::attach	(CSE_ALifeInventoryItem *tpALifeInventoryItem, bool bALifeRequest, bool bAddChildren)
{
	if (!bALifeRequest)
		return;

	tpALifeInventoryItem->base()->ID_Parent	= ID;

	if (!bAddChildren)
		return;

	R_ASSERT2			(std::find(children.begin(),children.end(),tpALifeInventoryItem->base()->ID) == children.end(),"Item is already inside the inventory");
	children.push_back	(tpALifeInventoryItem->base()->ID);
}

void CSE_ALifeDynamicObject::detach(CSE_ALifeInventoryItem* tpALifeInventoryItem, ALife::OBJECT_IT* I, bool bALifeRequest, bool bRemoveChildren)
{
	auto root							= this;
	if (!bALifeRequest)
		while (root->ID_Parent != u16_max)
			root						= ai().alife().objects().object(root->ID_Parent);
	
	VERIFY								(bALifeRequest ||
		ai().game_graph().vertex(root->m_tGraphID)->level_id() == alife().graph().level().level_id()
	);

	auto l_tpALifeDynamicObject1		= smart_cast<CSE_ALifeDynamicObject*>(tpALifeInventoryItem);
	R_ASSERT2							(l_tpALifeDynamicObject1, "Invalid children objects");
	l_tpALifeDynamicObject1->o_Position	= root->o_Position;
	l_tpALifeDynamicObject1->m_tNodeID	= root->m_tNodeID;
	l_tpALifeDynamicObject1->m_tGraphID	= root->m_tGraphID;
	l_tpALifeDynamicObject1->m_fDistance= root->m_fDistance;

	if (!bALifeRequest)
		return;

	tpALifeInventoryItem->base()->ID_Parent	= 0xffff;

	if (I)
	{
		children.erase					(*I);
		return;
	}

	if (bRemoveChildren)
		children.erase_data				(tpALifeInventoryItem->base()->ID);
}

void CSE_ALifeDynamicObject::add_online(const bool &update_registries)
{
	NET_Packet							tNetPacket;
	ClientID							clientID;
	clientID.set						(alife().server().GetServerClient() ? alife().server().GetServerClient()->ID.value() : 0);

	for (auto id : children)
	{
		//Alundaio:
		if (id == ai().alife().graph().actor()->ID)
			continue;
		//-Alundaio

		auto child						= ai().alife().objects().object(id);
		if (!child)
			continue;

		child->s_flags.or				(M_SPAWN_UPDATE);
		auto child_abstract				= smart_cast<CSE_Abstract*>(child);
		alife().server().entity_Destroy	(child_abstract);

		child->o_Position				= o_Position;
		child->m_tNodeID				= m_tNodeID;
		alife().server().Process_spawn	(tNetPacket, clientID, false, child_abstract);
		child->s_flags.and				(u16(-1) ^ M_SPAWN_UPDATE);
		child->m_bOnline				= true;

		child->add_online				(false);
	}

	if (update_registries)
	{
		alife().scheduled().remove		(this);
		alife().graph().remove			(this, m_tGraphID, false);
	}
}

void CSE_ALifeDynamicObject::add_offline(const xr_vector<ALife::_OBJECT_ID> &saved_children, const bool &update_registries)
{
	for (auto id : saved_children)
	{
		auto child						= ai().alife().objects().object(id, true);
		R_ASSERT						(child);

		if (!child->can_save())
		{
			alife().release				(child);
			continue;
		}

		child->clear_client_data		();
		alife().graph().add				(child, child->m_tGraphID, false);
//		alife().graph().attach			(*object,inventory_item,child->m_tGraphID,true);
		alife().graph().remove			(child, child->m_tGraphID);

		children.push_back				(child->ID);
		child->ID_Parent				= ID;
	}

	if (update_registries)
	{
		alife().scheduled().add			(this);
		alife().graph().add				(this, m_tGraphID, false);
	}
}
