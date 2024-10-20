#include "stdafx.h"
#include "inventory_item_amountable.h"
#include "Level.h"

MAmountable::MAmountable(CGameObject* obj) : CModule(obj)
{
	m_net_weight						= pSettings->r_float(O.cNameSect(), "net_weight");
	m_net_volume						= pSettings->r_float(O.cNameSect(), "net_volume");
	m_unlimited							= !!pSettings->r_BOOL(O.cNameSect(), "unlimited");
	m_depletion_speed					= pSettings->r_float(O.cNameSect(), "depletion_speed");
	m_capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	m_fAmount							= m_capacity;

	float net_cost						= pSettings->r_float(O.cNameSect(), "net_cost");
	m_net_cost							= (net_cost == -1.f) ? CInventoryItem::readBaseCost(*O.cNameSect()): net_cost;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAmountable::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto m								= se_obj->getModule<CSE_ALifeModuleAmountable>(save);
	if (save)
		m->m_amount						= m_fAmount;
	else if (m)
		m_fAmount						= m->m_amount;
}

float MAmountable::sSumItemData(EItemDataTypes type)
{
	switch (type)
	{
	case eWeight:
		return							m_net_weight * get_fill();
	case eVolume:
		return							m_net_volume * get_fill();
	case eCost:
		return							m_net_cost * (get_fill() - 1.f);
	default:
		FATAL							("wrong item data type");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAmountable::OnAmountChange()
{
	if (m_fAmount < 0.f)
		m_fAmount						= 0.f;
	else if (!m_unlimited && m_fAmount > Capacity())
		m_fAmount						= m_capacity;

	if (!Useful() && O.Local() && OnServer())
	{
		I->SetDropManual				(TRUE);
		O.DestroyObject					();
	}
}

bool MAmountable::Useful() const
{
	bool res							= I->Useful();
	res									&= !Empty() || fMore(I->Weight(), 0.f) || fMore(I->Volume(), 0.f);
	return								res;
}

void MAmountable::SetAmount(float val)
{
	m_fAmount							= val;
	OnAmountChange						();
}

void MAmountable::ChangeAmount(float delta)
{
	m_fAmount							+= delta;
	OnAmountChange						();
}

void MAmountable::SetFill(float val)
{
	m_fAmount							= val * Capacity();
	OnAmountChange						();
}

void MAmountable::ChangeFill(float delta)
{
	m_fAmount							+= delta * Capacity();
	OnAmountChange						();
}
