#include "stdafx.h"
#include "inventory_item_amountable.h"
#include "Level.h"

CAmountable::CAmountable(CGameObject* obj) : CModule(obj)
{
	m_net_weight						= pSettings->r_float(O.cNameSect(), "net_weight");
	m_net_volume						= pSettings->r_float(O.cNameSect(), "net_volume");
	m_unlimited							= !!pSettings->r_bool(O.cNameSect(), "unlimited");
	m_depletion_speed					= pSettings->r_float(O.cNameSect(), "depletion_speed");
	m_capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	m_fAmount							= m_capacity;

	float net_cost						= pSettings->r_float(O.cNameSect(), "net_cost");
	m_net_cost							= (net_cost == -1.f) ? CInventoryItem::ReadBaseCost(*O.cNameSect()): net_cost;

	m_item								= cast<PIItem>();
}

void CAmountable::OnAmountChange()
{
	if (m_fAmount < 0.f)
		m_fAmount						= 0.f;
	else if (!m_unlimited && m_fAmount > Capacity())
		m_fAmount						= m_capacity;

	if (!Useful() && O.Local() && OnServer())
	{
		m_item->SetDropManual			(TRUE);
		O.DestroyObject					();
	}
}

bool CAmountable::Useful() const
{
	bool res							= m_item->Useful();
	res									&= !Empty() || fMore(m_item->Weight(), 0.f) || fMore(m_item->Volume(), 0.f);
	return								res;
}

float CAmountable::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eGetAmount:
			return						m_fAmount;
		case eGetFill:
			return						Fill();
		case eGetBar:
			return						Full() ? -1.f : Fill();
		case eWeight:
			return						m_net_weight * Fill();
		case eVolume:
			return						m_net_volume * Fill();
		case eCost:
			return						m_net_cost * (Fill() - 1.f);
	}

	return								CModule::aboba(type, data, param);
}

void CAmountable::SetAmount(float val)
{
	m_fAmount							= val;
	OnAmountChange						();
}

void CAmountable::ChangeAmount(float delta)
{
	m_fAmount							+= delta;
	OnAmountChange						();
}

void CAmountable::SetFill(float val)
{
	m_fAmount							= val * Capacity();
	OnAmountChange						();
}

void CAmountable::ChangeFill(float delta)
{
	m_fAmount							+= delta * Capacity();
	OnAmountChange						();
}

void CAmountable::saveData(CSE_ALifeObject* se_obj)
{
	auto m								= se_obj->getModule<CSE_ALifeModuleAmountable>(true);
	m->amount							= m_fAmount;
}

void CAmountable::loadData(CSE_ALifeObject* se_obj)
{
	if (auto m = se_obj->getModule<CSE_ALifeModuleAmountable>(false))
		m_fAmount						= m->amount;
}
