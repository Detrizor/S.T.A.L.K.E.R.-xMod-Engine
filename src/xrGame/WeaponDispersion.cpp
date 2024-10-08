// WeaponDispersion.cpp: ������ ��� ��������
// 
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "weapon.h"
#include "inventoryowner.h"
#include "actor.h"
#include "inventory_item_impl.h"

#include "actoreffector.h"
#include "effectorshot.h"

//���������� 1, ���� ������ � �������� ��������� � >1 ���� ����������
float CWeapon::GetConditionDispersionFactor() const
{
	return (1.f + fireDispersionConditionFactor * (1.f - GetConditionToWork()));
}

float CWeapon::GetFireDispersion()
{
	return GetFireDispersion(m_cartridge.param_s.kDisp);
}

//������� ��������� (� ��������) ������ � ������ ������������� �������
float CWeapon::GetFireDispersion(float cartridge_k) 
{
	//���� ������� ���������, ��������� ������ � �������� �������
	float fire_disp = fireDispersionBase * m_muzzle_koefs.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
	
	//��������� ���������, �������� ����� ��������
	if (H_Parent())
	{
		const CInventoryOwner* pOwner	= smart_cast<const CInventoryOwner*>(H_Parent());
		fire_disp						+= pOwner->getWeaponDispersion();
	}

	return fire_disp;
}

//////////////////////////////////////////////////////////////////////////
// ��� ������� ������ ������
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
