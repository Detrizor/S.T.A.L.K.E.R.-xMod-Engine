#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"

bool xrServer::Process_event_reject(NET_Packet& P, u16 id_parent, u16 id_entity, bool straight)
{
	// Parse message
	CSE_Abstract* e_entity				= game->get_entity_from_eid	(id_entity);
	if (!e_entity )
	{
		Msg								("! ERROR on rejecting: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame);
		return							false;
	}
	CSE_Abstract* e_parent				= game->get_entity_from_eid	(id_parent);
	if (!e_parent )
	{
		Msg								("! ERROR on rejecting: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame);
		return							false;
	}

	if (!e_parent->children.contains(id_entity))
	{
		Msg								("! ERROR on rejecting: can't find children [%d] of parent [%d]", id_entity, e_parent->ID);
		return							false;
	}

	if (e_entity->ID_Parent == 0xffff)
	{
		Msg								("! ERROR on rejecting: can't detach independant object. entity[%s][%d], parent[%s][%d], section[%s]", e_entity->name_replace(), id_entity,
			e_parent->name_replace(), id_parent, e_entity->s_name.c_str());
		return							false;
	}

	if (e_entity->ID_Parent != id_parent)
	{
		Msg								("! ERROR on rejecting: e_entity->ID_Parent = [%d]  parent = [%d][%s]  entity_id = [%d]  frame = [%d]", e_entity->ID_Parent, id_parent,
			e_parent->name_replace(), id_entity, Device.dwFrame);
		return							false;
	}

	game->OnDetach						(id_parent, id_entity);
	e_entity->ID_Parent					= 0xffff; 
	e_parent->children.erase_data		(id_entity);
	emitEvent							(P, straight);
	
	return								true;
}
