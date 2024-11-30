#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrserver_objects.h"

void xrServer::Perform_transfer(NET_Packet &PR, NET_Packet &PT,	CSE_Abstract* what, CSE_Abstract* from, CSE_Abstract* to)
{
	// Sanity check
	R_ASSERT	(what && from && to);
	R_ASSERT	(from != to);
	R_ASSERT	(what->ID_Parent == from->ID);
	u32			time		= Device.dwTimeGlobal;

	// 1. Perform migration if need it
	if (from->owner != to->owner)	PerformMigration(what,from->owner,to->owner);
	//Log						("B");

	// 2. Detach "FROM"
	xr_vector<u16>& C			= from->children;
	xr_vector<u16>::iterator c	= std::find	(C.begin(),C.end(),what->ID);
	R_ASSERT				(C.end()!=c);
	C.erase					(c);
	PR.w_begin				(M_EVENT);
	PR.w_u32				(time);
	PR.w_u16				(GE_OWNERSHIP_REJECT);
	PR.w_u16				(from->ID);
	PR.w_u16				(what->ID);

	// 3. Attach "TO"
	what->ID_Parent			= to->ID;
	to->children.push_back	(what->ID);
	PT.w_begin				(M_EVENT);
	PT.w_u32				(time+1);
	PT.w_u16				(GE_OWNERSHIP_TAKE);
	PT.w_u16				(to->ID);
	PT.w_u16				(what->ID);

}

void xrServer::Perform_reject(u16 id_from, u16 id_what, EDestroyType destroy_type, bool straight)
{
	NET_Packet							packet;
	packet.w_begin						(M_EVENT);
	packet.w_u32						(0);
	packet.w_u16						(GE_OWNERSHIP_REJECT);
	packet.w_u16						(id_from);
	packet.w_u16						(id_what);
	packet.w_u8							(destroy_type);

	Process_event_reject				(packet, id_from, id_what, straight);
}

void xrServer::Perform_release(u16 id, bool straight)
{
	NET_Packet							packet;
	packet.w_begin						(M_EVENT);
	packet.w_u32						(0);
	packet.w_u16						(GE_DESTROY);
	packet.w_u16						(id);
	packet.w_u16						(release);

	Process_event_destroy				(packet, id, straight);
}
