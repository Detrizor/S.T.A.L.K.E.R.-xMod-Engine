#include "stdafx.h"
#include "weapon_chamber.h"
#include "addon_owner.h"
#include "WeaponMagazined.h"
#include "addon.h"

MAddon* CWeaponChamber::get() const
{
	if (empty())
		return							nullptr;
	return								m_slot->addons.back();
}

void CWeaponChamber::drop(MAddon* chamber) const
{
	chamber->O.transfer					(u16_max, true);
	//--xd should do proper throwing in according to animation position and velocity
}

void CWeaponChamber::create_shell(CWeaponAmmo* ammo) const
{
	ammo->DestroyObject					(true);
	//--xd to be implemented
}

CWeaponAmmo* CWeaponChamber::getAmmo() const
{
	if (auto addon = get())
		return							addon->O.scast<CWeaponAmmo*>();
	return								nullptr;
}

bool CWeaponChamber::empty() const
{
	return								m_slot->empty();
}

void CWeaponChamber::load() const
{
	if (!empty())
		unload							(eInventory);

	auto se_obj							= O.giveItem(O.m_cartridge.m_ammoSect.c_str(), O.m_cartridge.m_fCondition, true);
	auto object							= Level().Objects.net_Find(se_obj->ID);
	auto addon							= object->getModule<MAddon>();
	m_slot->parent_ao->finishAttaching	(addon, m_slot);
}

void CWeaponChamber::load_from(CWeaponAmmo* ammo) const
{
	if (ammo->Get(O.m_cartridge))
		load							();
}

void CWeaponChamber::load_from_mag() const
{
	if (O.get_cartridge_from_mag())
		load							();
}

void CWeaponChamber::reload(bool expended) const
{
	O.bMisfire							= false;
	O.m_cocked							= true;

	if (expended && O.m_bolt_catch && O.m_magazine && O.m_magazine->Empty())
		O.m_locked						= true;

	auto chamber						= get();
	auto chamber_ammo					= (chamber) ? chamber->O.scast<CWeaponAmmo*>() : nullptr;
	bool need_load						= false;

	if (O.get_cartridge_from_mag())
	{
		if (expended && chamber_ammo &&
			chamber_ammo->cNameSect() == O.m_cartridge.m_ammoSect &&
			chamber_ammo->GetCondition() == O.m_cartridge.m_fCondition)
			chamber						= nullptr;
		else
			need_load					= true;
	}

	if (chamber)
	{
		if (expended && chamber_ammo)
			chamber_ammo->DestroyObject	(true);
		else
			drop						(chamber);
	}
	
	if (need_load)
		load							();
}

void CWeaponChamber::unload(EUnloadDestination destination) const
{
	O.bMisfire							= false;
	O.m_cocked							= true;

	if (empty())
		return;

	auto chamber						= get();
	switch (destination)
	{
	case eDrop:
		drop							(chamber);
		break;
	case eMagazine:
		O.m_magazine->loadCartridge		(chamber->O.scast<CWeaponAmmo*>());
		break;
	case eInventory:
	{
		auto owner						= O.H_Parent()->scast<CInventoryOwner>();
		auto ammo						= chamber->O.scast<CWeaponAmmo*>();
		if (ammo)
		{
			CCartridge					cartridge;
			ammo->Get					(cartridge, false);
			if (owner->pushCartridge(cartridge))
			{
				ammo->DestroyObject		(true);
				return;
			}
		}
		chamber->O.transfer				(O.Parent->ID(), true);
		break;
	}
	}
}

void CWeaponChamber::consume() const
{
	auto heap							= getAmmo();
	heap->Get							(O.m_cartridge, false);
	if (O.fOneShotTime == 0.f)
	{
		O.m_cocked						= false;
		create_shell					(heap);
	}
	else
		reload							(true);
}
