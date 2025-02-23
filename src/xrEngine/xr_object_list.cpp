#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "xrSheduler.h"
#include "xr_object_list.h"
#include "std_classes.h"

#include "xr_object.h"
#include "../xrCore/net_utils.h"

#include "CustomHUD.h"

class fClassEQ
{
	CLASS_ID cls;
public:
	fClassEQ(CLASS_ID C) : cls(C) {};
	IC bool operator() (CObject* O) { return cls == O->CLS_ID; }
};
#ifdef DEBUG
BOOL debug_destroy = TRUE;
#endif

CObjectList::CObjectList() :
	m_owner_thread_id(GetCurrentThreadId())
{
	ZeroMemory(map_NETID, 0xffff * sizeof(CObject*));
}

CObjectList::~CObjectList()
{
	R_ASSERT(objects_active.empty());
	R_ASSERT(objects_sleeping.empty());
	R_ASSERT(destroy_queue.empty());
	//. R_ASSERT ( map_NETID.empty() );
}

CObject* CObjectList::FindObjectByName(shared_str name)
{
	for (Objects::iterator I = objects_active.begin(); I != objects_active.end(); I++)
		if ((*I)->cName().equal(name)) return (*I);
	for (Objects::iterator I = objects_sleeping.begin(); I != objects_sleeping.end(); I++)
		if ((*I)->cName().equal(name)) return (*I);
	return NULL;
}
CObject* CObjectList::FindObjectByName(LPCSTR name)
{
	return FindObjectByName(shared_str(name));
}

CObject* CObjectList::FindObjectByCLS_ID(CLASS_ID cls)
{
	{
		Objects::iterator O = std::find_if(objects_active.begin(), objects_active.end(), fClassEQ(cls));
		if (O != objects_active.end()) return *O;
	}
	{
		Objects::iterator O = std::find_if(objects_sleeping.begin(), objects_sleeping.end(), fClassEQ(cls));
		if (O != objects_sleeping.end()) return *O;
	}

	return NULL;
}

void CObjectList::o_activate(CObject* O)
{
	VERIFY								(O && O->processing_enabled());
	objects_sleeping.erase_data			(O);
	objects_active.push_back			(O);
}

void CObjectList::o_sleep(CObject* O)
{
	VERIFY								(O && !O->processing_enabled());
	objects_active.erase_data			(O);
	objects_sleeping.push_back			(O);
}

void CObjectList::Update()
{
	if (!Device.Paused())
	{
		auto& stat						= Device.Statistic;
		stat->UpdateClient_updated		= 0;
		stat->UpdateClient.Begin		();
		stat->UpdateClient_active		= objects_active.size();
		stat->UpdateClient_total		= objects_active.size() + objects_sleeping.size();

		for (auto& O : objects_active)
		{
			if (O->updateQuery())
			{
				++Device.Statistic->UpdateClient_updated;
				O->update				();
			}
		}

		stat->UpdateClient.End			();
	}

	// Destroy
	if (!destroy_queue.empty())
	{
		// Info
		if (g_pGameLevel->bReady)
		{
			for (auto& obj : objects_active)
				for (int it = destroy_queue.size() - 1; it >= 0; --it)
					obj->net_Relcase	(destroy_queue[it]);

			for (auto& obj : objects_sleeping)
				for (int it = destroy_queue.size() - 1; it >= 0; --it)
					obj->net_Relcase	(destroy_queue[it]);
		}

		for (int it = destroy_queue.size() - 1; it >= 0; --it)
			Sound->object_relcase		(destroy_queue[it]);

		for (auto& c : m_relcase_callbacks)
		{
			for (auto& obj : destroy_queue)
			{
				c.m_Callback			(obj);
				g_hud->net_Relcase		(obj);
			}
		}

		// Destroy
		for (int it = destroy_queue.size() - 1; it >= 0; --it)
		{
			CObject* O					= destroy_queue[it];
			O->net_Destroy				();
			Destroy						(O);
		}

		destroy_queue.clear				();
	}
}

void CObjectList::net_Register(CObject* O)
{
	R_ASSERT							(O);
	R_ASSERT							(O->ID() < 0xffff);
	map_NETID[O->ID()]					= O;
}

void CObjectList::net_Unregister(CObject* O)
{
	//demo_spectator can have 0xffff
	if (O->ID() < 0xffff)
		map_NETID[O->ID()]				= NULL;
}

int g_Dump_Export_Obj = 0;

u32 CObjectList::net_Export(NET_Packet* _Packet, u32 start, u32 max_object_size)
{
	if (g_Dump_Export_Obj) Msg("---- net_export --- ");

	NET_Packet& Packet = *_Packet;
	u32 position;
	for (; start < objects_active.size() + objects_sleeping.size(); start++)
	{
		CObject* P = (start < objects_active.size()) ? objects_active[start] : objects_sleeping[start - objects_active.size()];
		if (P->net_Relevant() && !P->getDestroy())
		{
			Packet.w_u16(u16(P->ID()));
			Packet.w_chunk_open8(position);
			//Msg ("cl_export: %d '%s'",P->ID(),*P->cName());
			P->net_Export(Packet);

#ifdef DEBUG
			u32 size = u32(Packet.w_tell() - position) - sizeof(u8);
			if (size >= 256)
			{
				Debug.fatal(DEBUG_INFO, "Object [%s][%d] exceed network-data limit\n size=%d, Pend=%d, Pstart=%d",
							*P->cName(), P->ID(), size, Packet.w_tell(), position);
			}
#endif
			if (g_Dump_Export_Obj)
			{
				u32 size = u32(Packet.w_tell() - position) - sizeof(u8);
				Msg("* %s : %d", *(P->cNameSect()), size);
			}
			Packet.w_chunk_close8(position);
			// if (0==(--count))
			// break;
			if (max_object_size >= (NET_PacketSizeLimit - Packet.w_tell()))
				break;
		}
	}
	if (g_Dump_Export_Obj) Msg("------------------- ");
	return start + 1;
}

int g_Dump_Import_Obj = 0;

void CObjectList::net_Import(NET_Packet* Packet)
{
	if (g_Dump_Import_Obj) Msg("---- net_import --- ");

	while (!Packet->r_eof())
	{
		u16 ID;
		Packet->r_u16(ID);
		u8 size;
		Packet->r_u8(size);
		CObject* P = net_Find(ID);
		if (P)
		{

			u32 rsize = Packet->r_tell();

			P->net_Import(*Packet);

			if (g_Dump_Import_Obj) Msg("* %s : %d - %d", *(P->cNameSect()), size, Packet->r_tell() - rsize);

		}
		else Packet->r_advance(size);
	}

	if (g_Dump_Import_Obj) Msg("------------------- ");
}

/*
CObject* CObjectList::net_Find(u16 ID)
{

xr_map<u32,CObject*>::iterator it = map_NETID.find(ID);
return (it==map_NETID.end())?0:it->second;
}
*/
void CObjectList::Load()
{
	R_ASSERT(/*map_NETID.empty() &&*/ objects_active.empty() && destroy_queue.empty() && objects_sleeping.empty());
}

void CObjectList::Unload()
{
	if (objects_sleeping.size() || objects_active.size())
		Msg("! objects-leaked: %d", objects_sleeping.size() + objects_active.size());

	// Destroy objects
	while (objects_sleeping.size())
	{
		CObject* O = objects_sleeping.back();
		Msg("! [%x] s[%4d]-[%s]-[%s]", O, O->ID(), *O->cNameSect(), *O->cName());
		O->setDestroy(true);

#ifdef DEBUG
		if (debug_destroy)
			Msg("Destroying object [%d][%s]", O->ID(), *O->cName());
#endif
		O->net_Destroy();
		Destroy(O);
	}
	while (objects_active.size())
	{
		CObject* O = objects_active.back();
		Msg("! [%x] a[%4d]-[%s]-[%s]", O, O->ID(), *O->cNameSect(), *O->cName());
		O->setDestroy(true);

#ifdef DEBUG
		if (debug_destroy)
			Msg("Destroying object [%d][%s]", O->ID(), *O->cName());
#endif
		O->net_Destroy();
		Destroy(O);
	}
}

CObject* CObjectList::Create(LPCSTR name)
{
	CObject* O = g_pGamePersistent->ObjectPool.create(name);
	// Msg("CObjectList::Create [%x]%s", O, name);
	objects_sleeping.push_back(O);
	return O;
}

void CObjectList::Destroy(CObject* O)
{
	if (!O)
		return;

	net_Unregister(O);

	// active/inactive
	auto _i = objects_active.find(O);
	if (_i != objects_active.end())
	{
		objects_active.erase(_i);
		VERIFY(!objects_active.contains(O));
		VERIFY(!objects_sleeping.contains(O));
	}
	else
	{
		Objects::iterator _ii = objects_sleeping.find(O);
		if (_ii != objects_sleeping.end())
		{
			objects_sleeping.erase(_ii);
			VERIFY(!objects_sleeping.contains(O));
		}
		else
			FATAL("! Unregistered object being destroyed");
	}

	g_pGamePersistent->ObjectPool.destroy(O);
}

void CObjectList::relcase_register(RELCASE_CALLBACK cb, int* ID)
{
#ifdef DEBUG
	RELCASE_CALLBACK_VEC::iterator It = std::find(m_relcase_callbacks.begin(),
										m_relcase_callbacks.end(),
										cb);
	VERIFY(It == m_relcase_callbacks.end());
#endif
	*ID = m_relcase_callbacks.size();
	m_relcase_callbacks.push_back(SRelcasePair(ID, cb));
}

void CObjectList::relcase_unregister(int* ID)
{
	VERIFY(m_relcase_callbacks[*ID].m_ID == ID);
	m_relcase_callbacks[*ID] = m_relcase_callbacks.back();
	*m_relcase_callbacks.back().m_ID = *ID;
	m_relcase_callbacks.pop_back();
}

void CObjectList::dump_list(Objects& v, LPCSTR reason)
{
	Objects::iterator it = v.begin();
	Objects::iterator it_e = v.end();
#ifdef DEBUG
	Msg("----------------dump_list [%s]", reason);
	for (; it != it_e; ++it)
		Msg("%x - name [%s] ID[%d] parent[%s] getDestroy()=[%s]",
			(*it),
			(*it)->cName().c_str(),
			(*it)->ID(),
			((*it)->H_Parent()) ? (*it)->H_Parent()->cName().c_str() : "",
			((*it)->getDestroy()) ? "yes" : "no");
#endif // #ifdef DEBUG
}

bool CObjectList::dump_all_objects()
{
	dump_list(destroy_queue, "destroy_queue");
	dump_list(objects_active, "objects_active");
	dump_list(objects_sleeping, "objects_sleeping");
	return false;
}

void CObjectList::register_object_to_destroy(CObject* object_to_destroy)
{
#ifdef DEBUG
	VERIFY(!registered_object_to_destroy(object_to_destroy));
#endif
	// Msg("CObjectList::register_object_to_destroy [%x]", object_to_destroy);
	destroy_queue.push_back(object_to_destroy);

	Objects::iterator it = objects_active.begin();
	Objects::iterator it_e = objects_active.end();
	for (; it != it_e; ++it)
	{
		CObject* O = *it;
		if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
		{
			Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(), O->ID(), Device.dwFrame);
			O->setDestroy(TRUE);
		}
	}

	it = objects_sleeping.begin();
	it_e = objects_sleeping.end();
	for (; it != it_e; ++it)
	{
		CObject* O = *it;
		if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
		{
			Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(), O->ID(), Device.dwFrame);
			O->setDestroy(TRUE);
		}
	}
}

#ifdef DEBUG
bool CObjectList::registered_object_to_destroy(const CObject* object_to_destroy) const
{
	return (
			   std::find(
				   destroy_queue.begin(),
				   destroy_queue.end(),
				   object_to_destroy
			   ) !=
			   destroy_queue.end()
		   );
}
#endif // DEBUG
