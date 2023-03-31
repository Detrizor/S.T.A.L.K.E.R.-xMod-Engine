#include "stdafx.h"
#include "inventory_item_amountable.h"
#include "Level.h"

CItemAmountable::CItemAmountable()
{
	m_item							= NULL;
	m_net_weight					= 0.f;
	m_net_volume					= 0.f;
	m_net_cost						= 0.f;
	m_capacity						= 1.f;
	m_unlimited						= false;
	m_depletion_speed				= 1.f;

	m_fAmount						= 1.f;
}

DLL_Pure* CItemAmountable::_construct()
{
	m_object						= smart_cast<CPhysicItem*>(this);
	m_item							= smart_cast<PIItem>(this);
	return							m_object;
}

void CItemAmountable::Load(LPCSTR section)
{
	m_net_weight					= pSettings->r_float(section, "net_weight");
	m_net_volume					= pSettings->r_float(section, "net_volume");
	m_depletion_speed				= pSettings->r_float(section, "depletion_speed");
	m_capacity						= pSettings->r_float(section, "capacity");
	m_fAmount						= m_capacity;
	m_unlimited						= !!pSettings->r_bool(section, "unlimited");
	float net_cost					= pSettings->r_float(section, "net_cost");
	m_net_cost						= (net_cost == -1.f) ? CInventoryItem::ReadBaseCost(section): net_cost;
}

bool CItemAmountable::Useful() const
{
	bool res						= m_item->Useful();
	res								&= !Empty() || fMore(m_item->Weight(), 0.f) || fMore(m_item->Volume(), 0.f);
	return							res;
}

void CItemAmountable::OnAmountChange()
{
	if (fLess(m_fAmount, 0.f))
		m_fAmount					= 0.f;
	else if (!m_unlimited && fMore(m_fAmount, Capacity()))
		m_fAmount					= m_capacity;

	if (!Useful() && m_object->Local() && OnServer())
		m_object->DestroyObject		();
}

void CItemAmountable::SetAmount(float val)
{
	m_fAmount						= val;
	OnAmountChange					();
}

void CItemAmountable::ChangeAmount(float delta)
{
	m_fAmount						+= delta;
	OnAmountChange					();
}

void CItemAmountable::SetFill(float val)
{
	m_fAmount						= val * Capacity();
	OnAmountChange					();
}

void CItemAmountable::ChangeFill(float delta)
{
	m_fAmount						+= delta * Capacity();
	OnAmountChange					();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DLL_Pure* CIIOAmountable::_construct()
{
	CItemAmountable::_construct		();
	inherited::_construct			();
	return							this;
}

void CIIOAmountable::Load(LPCSTR section)
{
	inherited::Load					(section);
	CItemAmountable::Load			(section);
}
