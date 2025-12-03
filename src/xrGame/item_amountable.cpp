#include "stdafx.h"
#include "item_amountable.h"

#include "Level.h"
#include "inventory_item_object.h"

MAmountable::MAmountable(CGameObject* obj) : CModule(obj),
	m_bUnlimited(!!pSettings->r_BOOL(O.cNameSect(), "unlimited")),
	m_fMaxAmount(pSettings->r_float(O.cNameSect(), "max_amount")),
	m_fNetWeight(pSettings->r_float(O.cNameSect(), "net_weight")),
	m_fNetVolume(pSettings->r_float(O.cNameSect(), "net_volume")),
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
		get_base_amount();
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
		return m_fNetCost * (get_fill() - 1.F);
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

void MAmountable::get_base_amount()
{
	float amount_mean{};
	if (pSettings->line_exist(O.cNameSect(), "base_amount"))
		amount_mean = pSettings->r_float(O.cNameSect(), "base_amount");
	else if (pSettings->line_exist(O.cNameSect(), "base_fill"))
		amount_mean = m_fMaxAmount * pSettings->r_float(O.cNameSect(), "base_fill");
	else
		amount_mean = m_fMaxAmount;

	float amount_dispersion{ pSettings->r_float_ex(O.cNameSect(), "base_amount_dispersion", 0.F) };
	if (!fIsZero(amount_dispersion))
	{
		const float min = amount_mean * (1.F - amount_dispersion);
		const float max = amount_mean * (1.F + amount_dispersion);
		m_fAmount = Random.randF(min, max);
	}
	else
		m_fAmount = amount_mean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
