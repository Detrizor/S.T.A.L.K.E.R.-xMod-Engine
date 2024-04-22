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

float CWeapon::GetFireDispersion(CCartridge* cartridge, bool for_crosshair)
{
	return GetFireDispersion((cartridge) ? cartridge->param_s.kDisp : 1.f, for_crosshair);
}

float CWeapon::GetBaseDispersion(float cartridge_k)
{
	return fireDispersionBase * m_silencer_koef.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
}

//������� ��������� (� ��������) ������ � ������ ������������� �������
float CWeapon::GetFireDispersion	(float cartridge_k, bool for_crosshair) 
{
	//���� ������� ���������, ��������� ������ � �������� �������
	float fire_disp = (for_crosshair) ? 0.f : GetBaseDispersion(cartridge_k);
	
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

void  CWeapon::RemoveShotEffector	()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_shot_remove	(this);
}

void	CWeapon::ClearShotEffector	()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_hide	(this);
}

void	CWeapon::StopShotEffector	()
{
	CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if (pInventoryOwner)
		pInventoryOwner->on_weapon_shot_stop();
}
