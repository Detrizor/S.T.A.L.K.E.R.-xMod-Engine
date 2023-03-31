#pragma once

#include "module.h"

class CInventoryStorage;
class CWeaponAmmo;
class CCartridge;
class CGameObject;

class CMagazine : public CModule
{
public:
							CMagazine				(CGameObject* obj);

	virtual void			Load					(LPCSTR section);

private:
	xr_vector<CWeaponAmmo*>	m_Heaps;
			u16				m_iNextHeapIdx;
			u16				m_iHeapsCount;
	xr_vector<shared_str>	m_ammo_types;
			u16				m_capacity;
			
			void			UpdateBulletsVisibility	();

protected:
	void								OnChild							(CObject* obj, bool take);

public:
			u16				Capacity				()								const	{ return m_capacity; };
			
			u16				Amount					()								const;
			bool			Empty					()								const;
			bool			CanTake					(CWeaponAmmo* ammo)				const;
			void			LoadCartridge			(CWeaponAmmo* ammo);
			bool			GetCartridge			(CCartridge& destination, bool expend = true);

	float								GetAmount							CO$	()		{ return (float)Amount(); }
	float								GetFill								CO$	()		{ return (float)Amount() / (float)Capacity(); }
	float								GetBar								CO$	()		{ return (Amount() < Capacity()) ? GetFill() : -1.f; }

	float								Weight								CO$	();
	float								Cost								CO$	();

	virtual void			UpdateHudBonesVisibility();
};
