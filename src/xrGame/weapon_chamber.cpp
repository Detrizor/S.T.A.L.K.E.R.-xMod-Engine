#include "stdafx.h"
#include "weapon_chamber.h"

#include "addon.h"
#include "addon_owner.h"
#include "WeaponMagazined.h"

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
	CSE_Abstract* se_shell				= nullptr;
	if (ammo->getShellSection())
		se_shell						= O.giveItem(ammo->getShellSection(), ammo->GetCondition(), true);
	ammo->DestroyObject					(true);
	if (se_shell)
	{
		auto shell						= Level().Objects.net_Find(se_shell->ID);
		auto addon						= shell->mcast<MAddon>();
		m_slot->parent_ao->finishAttaching(addon, m_slot);
	}
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

void CWeaponChamber::load(CCartridge CR$ cartridge) const
{
	if (!empty())
		unload							(eInventory);

	auto se_obj							= O.giveItem(cartridge.m_ammoSect.c_str(), cartridge.m_fCondition, true);
	auto object							= Level().Objects.net_Find(se_obj->ID);
	auto addon							= object->mcast<MAddon>();
	m_slot->parent_ao->finishAttaching	(addon, m_slot);
}

void CWeaponChamber::load(CWeaponAmmo* ammo) const
{
	if (auto cartridge = ammo->getCartridge())
		load							(cartridge.get());
}

void CWeaponChamber::load_from_mag() const
{
	if (auto cartridge = O.get_cartridge_from_mag())
		load							(cartridge.get());
}

void CWeaponChamber::reload(bool expended) const
{
	O.bMisfire							= false;
	O.m_cocked							= true;

	if (expended && O.m_bolt_catch && O.m_magazine && O.m_magazine->Empty())
		O.m_locked						= true;

	auto chamber						= get();
	auto next_cartridge					= O.get_cartridge_from_mag();

	if (expended && next_cartridge)
	{
		auto chamber_ammo				= chamber->O.scast<CWeaponAmmo*>();
		if (chamber_ammo->cNameSect() == next_cartridge->m_ammoSect &&
			chamber_ammo->GetCondition() == next_cartridge->m_fCondition)
			return;
	}

	if (chamber)
	{
		if (expended)
			chamber->O.DestroyObject	(true);
		else
			drop						(chamber);
	}
	
	if (next_cartridge)
		load							(next_cartridge.get());
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
		auto owner						= O.H_Parent()->scast<CInventoryOwner*>();
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
	auto heap{ getAmmo() };
	heap->Get(O.m_cartridge, false);
	if (O._selfLoading)
		reload(true);
	else
	{
		O.m_cocked = false;
		create_shell(heap);
	}
}
