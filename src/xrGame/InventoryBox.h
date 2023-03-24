#pragma once
#include "inventory_space.h"
#include "GameObject.h"
#include "script_export_space.h"

class CInventoryStorage
{
private:
			CGameObject*	m_object;

public:
							CInventoryStorage		();
	virtual					~CInventoryStorage		()										{};
	virtual DLL_Pure*		_construct				();
	
	virtual	void			OnEvent					(NET_Packet& P, u16 type);

private:
			TIItemContainer	m_items;

protected:
	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

	friend	class			CInventoryContainer;
};

class CInventoryContainer : public CInventoryStorage
{
private:
	typedef	CInventoryStorage inherited;

public:
							CInventoryContainer		();
	virtual					~CInventoryContainer	()										{}

	virtual	void			Load					(LPCSTR section);

private:
			float			m_capacity;
			
			void			OnInventoryAction		(PIItem item, u16 actionType)	const;
			
protected:
	IC		void			SetCapacity				(float val)								{ m_capacity = val; }

	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

public:
	IC const TIItemContainer&Items					()								const	{ return m_items; }
	IC		float			GetCapacity				()								const	{ return m_capacity; }
	IC		bool			Empty					()								const	{ return m_items.empty(); }

			bool			CanTakeItem				(PIItem iitem)					const;
			void			AddAvailableItems		(TIItemContainer& items_container) const;
			float			ItemsWeight				()								const;
			float			ItemsVolume				()								const;
};

class CInventoryBox : public CGameObject,
	public CInventoryContainer
{
private:
	typedef CGameObject	inherited;
	
public:
							CInventoryBox			();
	virtual					~CInventoryBox			()										{}
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);
	
	virtual	void			OnEvent					(NET_Packet& P, u16 type);
	virtual	BOOL			net_Spawn				(CSE_Abstract* DC);
	
private:
			bool			m_in_use;
			bool			m_can_take;
			bool			m_closed;
			
			void			SE_update_status		();
			
protected:
	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

public:
	IC		void			set_in_use				(bool status)							{ m_in_use = status; }
	IC		bool			in_use					()								const	{ return m_in_use; }
	IC		bool			can_take				()								const	{ return m_can_take; }
	IC		bool			closed					()								const	{ return m_closed; }
	
			void			set_closed				(bool status, LPCSTR reason);
			void			set_can_take			(bool status);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CInventoryBox)
#undef script_type_list
#define script_type_list save_type_list(CInventoryBox)