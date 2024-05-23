// WeaponDispersion.cpp: разбос при стрельбе
// 
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "weapon.h"
#include "inventoryowner.h"
#include "actor.h"
#include "inventory_item_impl.h"

#include "actoreffector.h"
#include "effectorshot.h"

//возвращает 1, если оружие в отличном состоянии и >1 если повреждено
float CWeapon::GetConditionDispersionFactor() const
{
	return (1.f + fireDispersionConditionFactor * (1.f - GetConditionToWork()));
}

float CWeapon::GetFireDispersion(CCartridge* cartridge)
{
	return GetFireDispersion(cartridge->param_s.kDisp);
}

//текущая дисперсия (в радианах) оружия с учетом используемого патрона
float CWeapon::GetFireDispersion(float cartridge_k) 
{
	//учет базовой дисперсии, состояние оружия и влияение патрона
	float fire_disp = fireDispersionBase * m_silencer_koef.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
	
	//вычислить дисперсию, вносимую самим стрелком
	if (H_Parent())
	{
		const CInventoryOwner* pOwner	= smart_cast<const CInventoryOwner*>(H_Parent());
		fire_disp						+= pOwner->getWeaponDispersion();
	}

	return fire_disp;
}

//////////////////////////////////////////////////////////////////////////
// Для эффекта отдачи оружия
void CWeapon::AddShotEffector		()
{
	inventory_owner().on_weapon_shot_start	(this);
}

void CWeapon::RemoveShotEffector	()
{
	if (auto pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent()))
		pInventoryOwner->on_weapon_shot_remove	(this);
}

void CWeapon::ClearShotEffector		()
{
	if (auto pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent()))
		pInventoryOwner->on_weapon_hide	(this);
}

void CWeapon::StopShotEffector		()
{
	if (auto pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent()))
		pInventoryOwner->on_weapon_shot_stop();
}
