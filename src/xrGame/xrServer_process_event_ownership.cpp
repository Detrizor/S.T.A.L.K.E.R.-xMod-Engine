#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer_svclient_validation.h"

void xrServer::Process_event_ownership(NET_Packet& P, u16 id_parent, u16 id_entity, bool straight)
{
	CSE_Abstract* e_parent				= game->get_entity_from_eid(id_parent);
	if (!e_parent)
	{
		Msg								("! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame);
		return;
	}
	
	CSE_Abstract* e_entity				= game->get_entity_from_eid(id_entity);
	if (!e_entity)
	{
		Msg								("! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame);
		return;
	}
	
	if (!is_object_valid_on_svclient(id_parent))
	{
		Msg								("! ERROR on ownership: parent object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, Device.dwFrame);
		return;
	}

	if (!is_object_valid_on_svclient(id_entity))
	{
		Msg								("! ERROR on ownership: entity object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, Device.dwFrame);
		return;
	}

	if (e_entity->ID_Parent != 0xffff)
	{
		Msg								("! ERROR on ownership: can't attach parented object. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame);
		return;
	}

	// Game allows ownership of entity
	if (game->OnTouch(id_parent, id_entity))
	{
		// Rebuild parentness
		e_entity->ID_Parent				= id_parent;
		e_parent->children.push_back	(id_entity);
		emitEvent						(P, straight);
	}
}
