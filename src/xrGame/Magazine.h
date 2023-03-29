#pragma once
#include "weaponammo.h"
#include "inventory_item_object.h"

#include "addon.h"

class CInventoryStorage;
class CWeaponAmmo;

class CMagazine
{
private:
	CGameObject*			m_object;

public:
							CMagazine				();
	virtual					~CMagazine				()										{}
	virtual DLL_Pure*		_construct				();

	virtual void			Load					(LPCSTR section);

private:
	xr_vector<CWeaponAmmo*>	m_Heaps;
			u16				m_iNextHeapIdx;
			u16				m_iHeapsCount;
	xr_vector<shared_str>	m_ammo_types;
			u16				m_capacity;
			
protected:
			void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

public:
			u16				Capacity				()								const	{ return m_capacity; };
			
			u16				Amount					()								const;
			bool			Empty					()								const;
			bool			CanTake					(CWeaponAmmo* ammo)				const;
			void			LoadCartridge			(CWeaponAmmo* ammo);
			bool			GetCartridge			(CCartridge& destination, bool expend = true);
	
	virtual	float			Weight					()								const;
	virtual	u32				Cost					()								const;
};

class CMagazineObject : public CInventoryItemObject,
	public CMagazine
{
private:
	typedef	CInventoryItemObject	inherited;

public:
							CMagazineObject			()										{}
	virtual					~CMagazineObject		()										{}
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);
	virtual	void			OnEvent					(NET_Packet& P, u16 type);

private:
			void			UpdateBulletsVisibility	();

protected:
	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);

public:
	
	virtual	float			GetAmount				()								const	{ return (float)Amount(); }
	virtual float			GetFill					()								const	{ return (float)Amount() / (float)Capacity(); }
	virtual	float			GetBar					()								const	{ return (Amount() < Capacity()) ? GetFill() : -1.f; }
	
	virtual void			UpdateHudBonesVisibility();
	virtual	float			Weight					()								const;
	virtual	u32				Cost					()								const;
};
