#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"

void xrServer::Process_event_destroy(NET_Packet& P, u16 id_dest, bool straight)
{
	// кто должен быть уничтожен
	CSE_Abstract* e_dest				= game->get_entity_from_eid(id_dest);
	if (!e_dest)
	{
		Msg								("! ERROR on destroy: [%d] not found on server", id_dest);
		return;
	}

	// check if we have children 
	while (!e_dest->children.empty())
		Perform_release					(e_dest->children.front(), straight);
	
	if (e_dest->ID_Parent != 0xffff)
		Perform_reject					(e_dest->ID_Parent, id_dest, release, straight);
	emitEvent							(P, straight);

	// Everything OK, so perform entity-destroy
	if (e_dest->m_bALifeControl && ai().get_alife())
	{
		game_sv_Single* _game			= smart_cast<game_sv_Single*>(game);
		VERIFY							(_game);
		if (ai().alife().objects().object(id_dest, true))
			_game->alife().release		(e_dest, false);
	}

	if (game)
		game->OnDestroyObject			(e_dest->ID);

	entity_Destroy						(e_dest);
}
