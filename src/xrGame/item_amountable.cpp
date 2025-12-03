#include "stdafx.h"
#include "item_amountable.h"

#include "inventory_item_object.h"
#include "Level.h"

MAmountable::MAmountable(CGameObject* obj) : CModule(obj)
{
	m_net_weight						= pSettings->r_float(O.cNameSect(), "net_weight");
	m_net_volume						= pSettings->r_float(O.cNameSect(), "net_volume");
	m_unlimited							= !!pSettings->r_BOOL(O.cNameSect(), "unlimited");
	m_depletion_speed					= pSettings->r_float(O.cNameSect(), "depletion_speed");
	m_capacity							= pSettings->r_float(O.cNameSect(), "capacity");

	m_net_cost							= pSettings->r_float(O.cNameSect(), "net_cost");
	if (m_net_cost <= 1.f)
		m_net_cost						*= CInventoryItem::readBaseCost(*O.cNameSect());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAmountable::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto m								= se_obj->getModule<CSE_ALifeModuleAmountable>(save);
	if (save)
		m->m_amount						= m_amount;
	else if (m)
		m_amount						= m->m_amount;
	else
		get_base_amount					();
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
	if (m_amount < 0.f)
		m_amount						= 0.f;
	else if (!m_unlimited && m_amount > m_capacity)
		m_amount						= m_capacity;

	if (!Useful() && O.Local() && OnServer())
	{
		I->SetDropManual				(TRUE);
		O.DestroyObject					();
	}
}

void MAmountable::get_base_amount()
{
	float								amount_mean;
	if (pSettings->line_exist(O.cNameSect(), "base_amount"))
		amount_mean						= pSettings->r_float(O.cNameSect(), "base_amount");
	else if (pSettings->line_exist(O.cNameSect(), "base_fill"))
		amount_mean						= m_capacity * pSettings->r_float(O.cNameSect(), "base_fill");
	else
		amount_mean						= m_capacity;

	if (float amount_dispersion = pSettings->r_float_ex(O.cNameSect(), "base_amount_dispersion", 0.f))
	{
		float min						= amount_mean * (1.f - amount_dispersion);
		float max						= amount_mean * (1.f + amount_dispersion);
		m_amount						= Random.randF(min, max);
	}
	else
		m_amount						= amount_mean;
}

bool MAmountable::Useful() const
{
	bool res							= I->Useful();
	res									&= !Empty() || fMore(I->Weight(), 0.f) || fMore(I->Volume(), 0.f);
	return								res;
}

void MAmountable::SetAmount(float val)
{
	m_amount							= val;
	OnAmountChange						();
}

void MAmountable::ChangeAmount(float delta)
{
	m_amount							+= delta;
	OnAmountChange						();
}

void MAmountable::SetFill(float val)
{
	m_amount							= val * m_capacity;
	OnAmountChange						();
}

void MAmountable::ChangeFill(float delta)
{
	m_amount							+= delta * m_capacity;
	OnAmountChange						();
}
