////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_switch_manager.cpp
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator switch manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_switch_manager.h"
#include "xrServer_Objects_ALife.h"
#include "alife_graph_registry.h"
#include "alife_object_registry.h"
#include "alife_schedule_registry.h"
#include "game_level_cross_table.h"
#include "xrserver.h"
#include "ai_space.h"
#include "level_graph.h"

#ifdef DEBUG
#	include "level.h"
#endif // DEBUG

using namespace ALife;

struct remove_non_savable_predicate {
	xrServer			*m_server;

	IC		 remove_non_savable_predicate(xrServer *server)
	{
		VERIFY			(server);
		m_server		= server;
	}

	IC	bool operator()	(const ALife::_OBJECT_ID &id) const
	{
		CSE_Abstract	*object = m_server->game->get_entity_from_eid(id);
		VERIFY			(object);
		CSE_ALifeObject	*alife_object = smart_cast<CSE_ALifeObject*>(object);
		VERIFY			(alife_object);
		return			(!alife_object->can_save());
	}
};

void CALifeSwitchManager::add_online(CSE_ALifeDynamicObject *object, bool update_registries)
{
	START_PROFILE						("ALife/switch/add_online")
	VERIFY								((ai().game_graph().vertex(object->m_tGraphID)->level_id() == graph().level().level_id()));

	object->m_bOnline					= true;
	CSE_Abstract* l_tpAbstract			= smart_cast<CSE_Abstract*>(object);
	server().entity_Destroy				(l_tpAbstract);
	object->s_flags.or					(M_SPAWN_UPDATE);
	server().Process_spawn				(NET_Packet(), ClientID(), false, l_tpAbstract);
	object->s_flags.and					(u16(-1) ^ M_SPAWN_UPDATE);
	object->add_online					(update_registries);

	STOP_PROFILE
}

void CALifeSwitchManager::remove_online(CSE_ALifeDynamicObject *object, bool update_registries)
{
	START_PROFILE						("ALife/switch/remove_online")

	object->m_bOnline					= false;
	OBJECT_VECTOR saved_chidren			= object->children;
	while (!object->children.empty())
	{
		CSE_Abstract* child				= server().game->get_entity_from_eid(object->children.back());
		R_ASSERT2						(child, make_string("child registered but not found [%d]", object->children.back()));
		server().Perform_reject			(object->ID, child->ID, xrServer::offline_switch);
		remove_online					(smart_cast<CSE_ALifeDynamicObject*>(child), false);
	}
	server().Perform_destroy			(object, xrServer::offline_switch);
	VERIFY								(object->children.empty());
	object->ID							= server().PerformIDgen(object->ID);
	object->add_offline					(saved_chidren, update_registries);

	STOP_PROFILE
}

void CALifeSwitchManager::switch_online(CSE_ALifeDynamicObject *object)
{
	START_PROFILE("ALife/switch/switch_online")
#ifdef DEBUG
//	if (psAI_Flags.test(aiALife))
		Msg						("[LSS][%d] Going online [%d][%s][%d] ([%f][%f][%f] : [%f][%f][%f]), on '%s'",Device.dwFrame,Device.dwTimeGlobal,object->name_replace(), object->ID,VPUSH(graph().actor()->o_Position),VPUSH(object->o_Position), "*SERVER*");
#endif
	object->switch_online		();
	STOP_PROFILE
}

void CALifeSwitchManager::switch_offline(CSE_ALifeDynamicObject *object)
{
	START_PROFILE("ALife/switch/switch_offline")
#ifdef DEBUG
//	if (psAI_Flags.test(aiALife))
		Msg							("[LSS][%d] Going offline [%d][%s][%d] ([%f][%f][%f] : [%f][%f][%f]), on '%s'",Device.dwFrame,Device.dwTimeGlobal,object->name_replace(), object->ID,VPUSH(graph().actor()->o_Position),VPUSH(object->o_Position), "*SERVER*");
#endif
	object->switch_offline			();
	STOP_PROFILE
}

bool CALifeSwitchManager::synchronize_location(CSE_ALifeDynamicObject *I)
{
	START_PROFILE("ALife/switch/synchronize_location")
#ifdef DEBUG
	VERIFY3					(ai().level_graph().level_id() == ai().game_graph().vertex(I->m_tGraphID)->level_id(),*I->s_name,I->name_replace());
	if (!I->children.empty()) {
		u32					size = I->children.size();
		ALife::_OBJECT_ID	*test = (ALife::_OBJECT_ID*)_alloca(size*sizeof(ALife::_OBJECT_ID));
		Memory.mem_copy		(test,&*I->children.begin(),size*sizeof(ALife::_OBJECT_ID));
		std::sort			(test,test + size);
		for (u32 i=1; i<size; ++i) {
			VERIFY3			(test[i - 1] != test[i],"Child is registered twice in the child list",(*I).name_replace());
		}
	}
#endif // DEBUG

	// check if we do not use ai locations
	if (!I->used_ai_locations())
		return				(true);

	// check if we are not attached
	if (0xffff != I->ID_Parent)
		return				(true);

	// check if we are not online and have an invalid level vertex id
	if	(!I->m_bOnline && !ai().level_graph().valid_vertex_id(I->m_tNodeID))
		return				(true);

	return					((*I).synchronize_location());
	STOP_PROFILE
}

void CALifeSwitchManager::try_switch_online	(CSE_ALifeDynamicObject	*I)
{
	START_PROFILE("ALife/switch/try_switch_online")

	if (I->ID_Parent != u16_max)
	{
		auto parent = objects().object(I->ID_Parent);
		R_ASSERT(parent);
		if (!parent->m_bOnline)
			return;
	}

	I->try_switch_online();
	if (!I->m_bOnline && !I->keep_saved_data_anyway())
		I->clear_client_data();

	STOP_PROFILE
}

void CALifeSwitchManager::try_switch_offline(CSE_ALifeDynamicObject	*I)
{
	START_PROFILE("ALife/switch/try_switch_offline")

	if (I->ID_Parent == u16_max)
		I->try_switch_offline();

	STOP_PROFILE
}

void CALifeSwitchManager::switch_object	(CSE_ALifeDynamicObject	*I)
{
	if (I->redundant()) {
		release				(I);
		return;
	}

	if (!synchronize_location(I))
		return;

	if (I->m_bOnline)
		try_switch_offline	(I);
	else
		try_switch_online	(I);

	if (I->redundant())
		release				(I);
}
