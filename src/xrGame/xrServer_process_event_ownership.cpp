#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer_svclient_validation.h"

void ReplaceOwnershipHeader	(NET_Packet& P)
{
	//������ ����� ������, �� �� ������ ������ ����� ������ ���. ������� ������� ���������
	u16 NewType = GE_OWNERSHIP_TAKE;
	CopyMemory(&P.B.data[6],&NewType,2);
};

void xrServer::Process_event_ownership(NET_Packet& P, ClientID sender, u32 time, u16 ID, BOOL bForced)
{
	u32 MODE			= net_flags(TRUE,TRUE, FALSE, TRUE);

	u16					id_parent=ID,id_entity;
	P.r_u16				(id_entity);
	CSE_Abstract*		e_parent	= game->get_entity_from_eid	(id_parent);
	CSE_Abstract*		e_entity	= game->get_entity_from_eid	(id_entity);
	
	if ( !e_parent ) {
		Msg( "! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame );
		return;
	}
	if ( !e_entity ) {
//		Msg( "! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent, id_entity, Device.dwFrame );
		return;
	}
	
	if (!is_object_valid_on_svclient(id_parent))
	{
		Msg( "! ERROR on ownership: parent object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, Device.dwFrame );
		return;
	}

	if (!is_object_valid_on_svclient(id_entity))
	{
		Msg( "! ERROR on ownership: entity object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent, id_entity, Device.dwFrame );
		return;
	}

	if (0xffff != e_entity->ID_Parent)	return;

	xrClientData*		c_parent		= e_parent->owner;
	xrClientData*		c_entity		= e_entity->owner;
	xrClientData*		c_from			= ID_to_client	(sender);

	if ( (GetServerClient() != c_from) && (c_parent != c_from) )
	{
		// trust only ServerClient or new_ownerClient
		return;
	}

	// Game allows ownership of entity
	if (game->OnTouch	(id_parent,id_entity, bForced))
	{
		// Perform migration if needed
		if (c_parent != c_entity)		PerformMigration(e_entity,c_entity,c_parent);

		// Rebuild parentness
		e_entity->ID_Parent			= id_parent;
		e_parent->children.push_back(id_entity);

		if (bForced)
		{
			ReplaceOwnershipHeader(P);
		}
		// Signal to everyone (including sender)
		SendBroadcast		(BroadcastCID,P,MODE);
	}

}
