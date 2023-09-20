#pragma once

#include "inventory_item_object.h"

class CBackpack : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject inherited;

	float		m_capacity;

public:
							CBackpack();
	virtual					~CBackpack();

	virtual void			Load(LPCSTR section);

	virtual void			Hit(float P, ALife::EHitType hit_type);

	virtual void			OnMoveToSlot(const SInvItemPlace& prev);
	virtual void			OnMoveToRuck(const SInvItemPlace& previous_place);
	virtual void			OnH_A_Chield();

	virtual BOOL			net_Spawn(CSE_Abstract* DC);
	virtual void			net_Export(NET_Packet& P);
	virtual void			net_Import(NET_Packet& P);

			float			GetCapacity()					{ return m_capacity; }
			void			SetCapacity(float v)			{ m_capacity = v; }

protected:
	virtual bool			install_upgrade_impl(LPCSTR section, bool test);
};