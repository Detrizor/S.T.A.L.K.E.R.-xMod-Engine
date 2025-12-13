#include "stdafx.h"
#include "artefact_module.h"

#include "Artefact.h"
#include "GameObject.h"
#include "item_amountable.h"
#include "inventory_item_object.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MArtefactModule::MArtefactModule(CGameObject* obj) : CModule(obj),
	m_pAmountable(O.getModule<MAmountable>()),
	m_fArtefactActivateCharge(pSettings->r_float(O.cNameSect(), "artefact_activate_charge")),
	m_nModesCount(pSettings->r_u32(O.cNameSect(), "modes_count"))
{
	R_ASSERT2(m_pAmountable, "MArtefactModule requires MAmountable!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MArtefactModule::sSyncData(CSE_ALifeDynamicObject* /*se_obj*/, bool save)
{
	if (!save)
		setMode(static_cast<int>(round(m_pAmountable->getAmount() * static_cast<float>(m_nModesCount))));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float MArtefactModule::getAllowedArtefactCharge(const CArtefact* artefact) const
{
	const float target_radiation{ m_pAmountable->getAmount() };
	const float target_charge{ target_radiation * artefact->getMaxCharge() };
	return std::min(target_charge, m_fArtefactActivateCharge);
}

void MArtefactModule::setMode(int val)
{
	m_nCurMode = val;
	clamp(m_nCurMode, 0, m_nModesCount);
	m_pAmountable->setAmount(static_cast<float>(val) / static_cast<float>(m_nModesCount));
	I->SetInvIconType(m_nCurMode);
}
