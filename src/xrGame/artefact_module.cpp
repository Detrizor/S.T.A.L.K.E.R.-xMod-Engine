#include "stdafx.h"
#include "artefact_module.h"

#include "Artefact.h"
#include "GameObject.h"
#include "item_amountable.h"
#include "inventory_item_object.h"

#include "ui/UICellCustomItems.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MArtefactModule::MArtefactModule(CGameObject* obj) : CModule(obj),
	m_fArtefactActivateCharge(pSettings->r_float(O.cNameSect(), "artefact_activate_charge"))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MArtefactModule::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto pModule{ se_obj->getModule<CSE_ALifeModuleArtefactModule>(save) };
	if (save)
		pModule->m_fMode = m_fMode;
	else if (pModule)
		m_fMode = pModule->m_fMode;
}

xptr<CUICellItem> MArtefactModule::sCreateIcon()
{
	auto res{ xptr<CUICellItem>::create<CUIArtefactModuleCellItem>(this) };
	m_pIcon = smart_cast<CUIArtefactModuleCellItem*>(res.get());
	m_pIcon->setMode(m_fMode);
	return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float MArtefactModule::getAllowedArtefactCharge(const CArtefact* artefact) const
{
	const float target_radiation{ m_fMode };
	const float target_charge{ target_radiation * artefact->getMaxCharge() };
	return std::min(target_charge, m_fArtefactActivateCharge);
}

void MArtefactModule::setMode(float fValue)
{
	m_fMode = fValue;
	clamp(m_fMode, 0.F, 1.F);
	m_pIcon->setMode(m_fMode);
}
