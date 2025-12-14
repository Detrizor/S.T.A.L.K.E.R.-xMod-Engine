#include "stdafx.h"
#include "CUIArtefactParams.h"

#include "UIXmlInit.h"
#include "CUIMiscInfoItem.h"

#include "Artefact.h"
#include "string_table.h"
#include "items_library.h"

static const auto redClr{ color_argb(255, 210, 50, 50) };
static const auto greenClr{ color_argb(255, 170, 170, 170) };
static float maxRestores[]{ flt_min, flt_min, flt_min, flt_min };

static void InitMaxArtValues()
{
	for (auto const& sec : CItemsLibrary::getDivision("artefact", "active", "void"))
	{
		for (size_t i = 0; i < CArtefact::eRestoreTypeMax; ++i)
		{
			float fValue{ pSettings->r_float(sec, CArtefact::strRestoreNames[i]) };
			if (fValue > maxRestores[i])
				maxRestores[i] = fValue;
		}
	}
}

CUIArtefactParams::CUIArtefactParams() = default;
CUIArtefactParams::~CUIArtefactParams() = default;

void CUIArtefactParams::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "af_params" };

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strBaseNode, 0));

	m_pRadiation->init(xmlDoc, "radiation", false);

	for (size_t i = 0; i < CArtefact::eAbsorbationTypeMax; ++i)
		m_pAbsorbations[i]->init(xmlDoc, CArtefact::strAbsorbationNames[i], false);

	for (size_t i = 0; i < CArtefact::eRestoreTypeMax; ++i)
		m_pRestores[i]->init(xmlDoc, CArtefact::strRestoreNames[i], false);

	m_pWeightDump->init(xmlDoc, "weight_dump", false);
	m_pArmor->init(xmlDoc, "armor", false);

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIArtefactParams::setInfo(CArtefact* pArtefact, LPCSTR strSection)
{
	DetachAll();
	float fValue;
	float h{ 0.F };

	fValue = (pArtefact) ? pArtefact->getRadiation() : pSettings->r_float(strSection, "radiation");
	if (!fIsZero(fValue))
		m_pRadiation->setValue(fValue, h);

	for (size_t i = 0; i < CArtefact::eAbsorbationTypeMax; ++i)
	{
		fValue = (pArtefact) ? pArtefact->getAbsorbation(i) : pSettings->r_float(strSection, CArtefact::strAbsorbationNames[i]);
		if (!fIsZero(fValue))
			m_pAbsorbations[i]->setValue(fValue, h);
	}

	for (size_t i = 0; i < CArtefact::eRestoreTypeMax; ++i)
	{
		fValue = (pArtefact) ? pArtefact->getRestore(static_cast<CArtefact::EConditionRestoreTypes>(i)) : pSettings->r_float(strSection, CArtefact::strRestoreNames[i]);
		if (fIsZero(fValue))
			continue;

		if (maxRestores[i] == flt_min)
			InitMaxArtValues();
		fValue /= maxRestores[i];

		m_pRestores[i]->setValue(fValue, h);
	}

	fValue = (pArtefact) ? pArtefact->getWeightDump() : pSettings->r_float(strSection, "weight_dump");
	if (!fIsZero(fValue))
		m_pWeightDump->setValue(fValue, h);

	fValue = (pArtefact) ? pArtefact->getArmor() : pSettings->r_float(strSection, "armor");
	if (!fIsZero(fValue))
		m_pArmor->setValue(fValue, h);

	SetHeight(h);
}
