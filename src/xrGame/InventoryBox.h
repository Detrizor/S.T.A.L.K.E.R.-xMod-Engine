#pragma once
#include "inventory_space.h"
#include "GameObject.h"
#include "script_export_space.h"

class CInventoryBox :public CGameObject
{
	typedef CGameObject	inherited;

public:
	xr_vector<u16>		m_items;

protected:
	bool	m_in_use;
	bool	m_can_take;
	bool	m_closed;
	float	m_capacity;

public:
						CInventoryBox					();
	virtual				~CInventoryBox					();

	virtual		void	Load							(LPCSTR section);

	virtual		void	OnEvent							(NET_Packet& P, u16 type);
	virtual		BOOL	net_Spawn						(CSE_Abstract* DC);
	virtual		void	net_Destroy						();
	virtual		void	net_Relcase						(CObject* O);
				void	AddAvailableItems				(TIItemContainer& items_container) const;
	IC			bool	IsEmpty							() const {return m_items.empty();}
	virtual		void	UpdateCL						();

	IC			void	set_in_use						(bool status) { m_in_use = status; }
	IC			bool	in_use							() const	{ return m_in_use; }
				
				void	set_can_take					(bool status);
	IC			bool	can_take						() const	{ return m_can_take; }

				void	set_closed						(bool status, LPCSTR reason);
	IC			bool	closed							() const	{ return m_closed; }

				float	CalcItemsWeight					();
				float	CalcItemsVolume					();
				float	GetCapacity						()			{ return m_capacity; }
				void	SetCapacity						(float v)	{ m_capacity = v; }

protected:
				void	SE_update_status				();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CInventoryBox)
#undef script_type_list
#define script_type_list save_type_list(CInventoryBox)