#pragma once
#include "GameObject.h"
#include "item_container.h"
#include "script_export_space.h"

class CInventoryBox : public CGameObject
{
private:
	typedef CGameObject	inherited;
	
public:
							CInventoryBox			();
	
	virtual	BOOL			net_Spawn				(CSE_Abstract* DC);
	
private:
			bool			m_in_use;
			bool			m_can_take;
			bool			m_closed;
			
			void			SE_update_status		();

public:
	IC		void			set_in_use				(bool status)							{ m_in_use = status; }
	IC		bool			in_use					()								const	{ return m_in_use; }
	IC		bool			can_take				()								const	{ return m_can_take; }
	IC		bool			closed					()								const	{ return m_closed; }
	
			void			set_closed				(bool status, LPCSTR reason);
			void			set_can_take			(bool status);

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	float								Aboba								O$	(EEventTypes type, void* data, int param);

public:
	CInventoryContainer*				m_pContainer;
};

add_to_type_list(CInventoryBox)
#undef script_type_list
#define script_type_list save_type_list(CInventoryBox)
