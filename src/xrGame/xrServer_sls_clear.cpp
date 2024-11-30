#include "stdafx.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "xrServer_Objects.h"
#include "xrServer.h"
#include "xrmessages.h"
#include "ai_space.h"

void xrServer::Perform_destroy(CSE_Abstract* object, EDestroyType destroy_type, bool straight)
{
	R_ASSERT							(object);
	R_ASSERT							(object->ID_Parent == 0xffff);

	while (!object->children.empty())
	{
		CSE_Abstract* child				= game->get_entity_from_eid(object->children.back());
		R_ASSERT2						(child, make_string("child registered but not found [%d]", object->children.back()));
		Perform_reject					(object->ID, child->ID, destroy_type, straight);
		Perform_destroy					(child, destroy_type, straight);
	}

	u16 object_id						= object->ID;
	entity_Destroy						(object);

	NET_Packet							packet;
	packet.w_begin						(M_EVENT);
	packet.w_u32						(0);
	packet.w_u16						(GE_DESTROY);
	packet.w_u16						(object_id);
	packet.w_u16						(destroy_type);
	emitEvent							(packet, straight);
}

void xrServer::SLS_Clear()
{
	while (!entities.empty())
	{
		bool found						= false;
		for (auto& pair : entities)
		{
			if (pair.second->ID_Parent != 0xffff)
				continue;
			found						= true;
			Perform_destroy				(pair.second, sls_clear);
			break;
		}
		if (!found)		//R_ASSERT(found);
		{
			for (auto& pair : entities)
			{
				if (pair.second)
					Msg					("! ERROR: can't destroy object [%d][%s] with parent [%d]",
						pair.second->ID,
						pair.second->s_name.size() ? pair.second->s_name.c_str() : "unknown",
						pair.second->ID_Parent);
				else
					Msg					("! ERROR: can't destroy entity [%d][?] with parent[?]", pair.first);

			}
			Msg							("! ERROR: FATAL: can't delete all entities !");
			entities.clear				();
		}
		
	}
}
