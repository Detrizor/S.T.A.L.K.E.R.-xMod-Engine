#include "stdafx.h"
#include "artefact_module.h"

#include "GameObject.h"
#include "item_amountable.h"
#include "inventory_item_object.h"

MArtefactModule::MArtefactModule(CGameObject* obj) : CModule(obj),
	m_amountable_ptr(O.getModule<MAmountable>()),
	m_max_artefact_radiation_limit(pSettings->r_float(O.cNameSect(), "max_artefact_radiation_limit")),
	m_modes_count(pSettings->r_u32(O.cNameSect(), "modes_count"))
{
	R_ASSERT2							(m_amountable_ptr, "MArtefactModule requires MAmountable!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MArtefactModule::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	if (!save)
	{
		float amount					= m_amountable_ptr->getAmount();
		setMode							(static_cast<int>(round(amount * m_modes_count)));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float MArtefactModule::getArtefactRadiationLimit() const
{
	return								m_amountable_ptr->getAmount();
}

void MArtefactModule::setMode(int val)
{
	m_cur_mode							= val;
	clamp								(m_cur_mode, 0, m_modes_count);
	m_amountable_ptr->setAmount			(static_cast<float>(val) / static_cast<float>(m_modes_count));
	I->SetInvIconType					(m_cur_mode);
}
