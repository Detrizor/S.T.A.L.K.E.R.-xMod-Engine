#include "stdafx.h"
#include "item_amountable.h"

#include "Level.h"
#include "inventory_item_object.h"

MAmountable::MAmountable(CGameObject* obj) : CModule(obj),
	m_bUnlimited(pSettings->r_bool(O.cNameSect(), "unlimited")),
	m_bNetCostAdditive(pSettings->r_bool(O.cNameSect(), "net_cost_additive")),
	m_fMaxAmount(pSettings->r_float(O.cNameSect(), "max_amount")),
	m_fNetWeight(pSettings->r_float(O.cNameSect(), "net_weight")),
	m_fNetVolume(pSettings->r_float(O.cNameSect(), "net_volume")),
	m_fAmountCost(pSettings->r_float(O.cNameSect(), "amount_cost")),
	m_fNetCost(pSettings->r_float(O.cNameSect(), "net_cost")),
	m_fDepletionSpeed(pSettings->r_float(O.cNameSect(), "depletion_speed"))
{
	if (fLessOrEqual(m_fNetCost, 1.F))
		m_fNetCost *= CInventoryItem::readBaseCost(*O.cNameSect());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MAmountable::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto m = se_obj->getModule<CSE_ALifeModuleAmountable>(save);
	if (save)
		m->m_amount = m_fAmount;
	else if (m)
		m_fAmount = m->m_amount;
	else
		m_fAmount = getBaseAmount(O.cNameSect());
}

float MAmountable::sSumItemData(EItemDataTypes type)
{
	switch (type)
	{
	case eWeight:
		return m_fNetWeight * get_fill();
	case eVolume:
		return m_fNetVolume * get_fill();
	case eCost:
		if (!fIsZero(m_fAmountCost))
			return m_fAmountCost * ((m_bNetCostAdditive) ? m_fAmount : m_fAmount - m_fMaxAmount);
		if (!fIsZero(m_fNetCost))
			return m_fNetCost * ((m_bNetCostAdditive) ? get_fill() : get_fill() - 1.F);
		return 0.F;
	default:
		FATAL("wrong item data type");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool MAmountable::is_useful() const
{
	bool res = I->Useful();
	res &= !Empty() || fMore(I->Weight(), 0.F) || fMore(I->Volume(), 0.F);
	return res;
}

void MAmountable::on_amount_change()
{
	if (m_fAmount < 0.F)
		m_fAmount = 0.F;
	else if (!m_bUnlimited && m_fAmount > m_fMaxAmount)
		m_fAmount = m_fMaxAmount;

	if (!is_useful() && O.Local() && OnServer())
	{
		I->SetDropManual(TRUE);
		O.DestroyObject();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float MAmountable::getBaseAmount(const shared_str& section)
{
	float amount_mean;
	if (pSettings->line_exist(section, "base_amount"))
		amount_mean = pSettings->r_float(section, "base_amount");
	else if (pSettings->line_exist(section, "base_fill"))
		amount_mean = pSettings->r_float(section, "max_amount") * pSettings->r_float(section, "base_fill");
	else
		amount_mean = (pSettings->r_bool(section, "net_cost_additive")) ? 0.F : pSettings->r_float(section, "max_amount");

	float amount_dispersion{ pSettings->r_float_ex(section, "base_amount_dispersion", 0.F) };
	if (!fIsZero(amount_dispersion))
	{
		const float min = amount_mean * (1.F - amount_dispersion);
		const float max = amount_mean * (1.F + amount_dispersion);
		return Random.randF(min, max);
	}

	return amount_mean;
}

void MAmountable::setAmount(float val)
{
	m_fAmount = val;
	on_amount_change();
}

void MAmountable::changeAmount(float delta)
{
	m_fAmount += delta;
	on_amount_change();
}

void MAmountable::setFill(float val)
{
	m_fAmount = val * m_fMaxAmount;
	on_amount_change();
}

void MAmountable::changeFill(float delta)
{
	m_fAmount += delta * m_fMaxAmount;
	on_amount_change();
}
