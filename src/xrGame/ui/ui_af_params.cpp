#include "stdafx.h"
#include "ui_af_params.h"

#include "UIXmlInit.h"
#include "CUIMiscInfoItem.h"

#include "Artefact.h"
#include "string_table.h"
#include "items_library.h"

static const auto redClr{ color_argb(255, 210, 50, 50) };
static const auto greenClr{ color_argb(255, 170, 170, 170) };
static float maxRestores[]{ flt_min, flt_min, flt_min, flt_min };

static const LPCSTR strAbsorbationCaptions[] =  // EImmunityTypes
{
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
};

static const LPCSTR strRestoreCaption[] =  // EConditionRestoreTypes
{
	"ui_inv_radiation",
	"ui_inv_painkill",
	"ui_inv_regeneration",
	"ui_inv_recuperation"
};

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

	m_pRadiation->init(xmlDoc, "radiation");
	m_pRadiation->SetCaption(CStringTable().translate("st_radiation").c_str());

	for (size_t i = 0; i < CArtefact::eAbsorbationTypeMax; ++i)
	{
		m_pAbsorbations[i]->init(xmlDoc, CArtefact::strAbsorbationNames[i]);
		m_pAbsorbations[i]->SetCaption(CStringTable().translate(strAbsorbationCaptions[i]).c_str());
	}

	for (size_t i = 0; i < CArtefact::eRestoreTypeMax; ++i)
	{
		m_pRestores[i]->init(xmlDoc, CArtefact::strRestoreNames[i]);
		m_pRestores[i]->SetCaption(CStringTable().translate(strRestoreCaption[i]).c_str());
	}

	m_pWeightDump->init(xmlDoc, "weight_dump");
	m_pWeightDump->SetCaption(CStringTable().translate("st_weight_dump").c_str());

	m_pArmor->init(xmlDoc, "armor");
	m_pArmor->SetCaption(CStringTable().translate("st_armor").c_str());

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIArtefactParams::set_info_item(CUIMiscInfoItem* item, float value, float& h)
{
	item->SetValue(value);
	item->SetY(h);
	h += item->GetWndSize().y;
	AttachChild(item);
}

void CUIArtefactParams::setInfo(CArtefact* pArtefact, LPCSTR strSection)
{
	DetachAll();
	float fValue;
	float h{ 0.F };

	fValue = (pArtefact) ? pArtefact->getRadiation() : pSettings->r_float(strSection, "radiation");
	if (!fIsZero(fValue))
		set_info_item(m_pRadiation.get(), fValue, h);

	for (size_t i = 0; i < CArtefact::eAbsorbationTypeMax; ++i)
	{
		fValue = (pArtefact) ? pArtefact->getAbsorbation(i) : pSettings->r_float(strSection, CArtefact::strAbsorbationNames[i]);
		if (!fIsZero(fValue))
			set_info_item(m_pAbsorbations[i].get(), fValue, h);
	}

	for (size_t i = 0; i < CArtefact::eRestoreTypeMax; ++i)
	{
		fValue = (pArtefact) ? pArtefact->getRestore(static_cast<CArtefact::EConditionRestoreTypes>(i)) : pSettings->r_float(strSection, CArtefact::strRestoreNames[i]);
		if (fIsZero(fValue))
			continue;

		if (maxRestores[i] == flt_min)
			InitMaxArtValues();
		fValue /= maxRestores[i];

		set_info_item(m_pRestores[i].get(), fValue, h);
	}

	fValue = (pArtefact) ? pArtefact->getWeightDump() : pSettings->r_float(strSection, "weight_dump");
	if (!fIsZero(fValue))
		set_info_item(m_pWeightDump.get(), fValue, h);

	fValue = (pArtefact) ? pArtefact->getArmor() : pSettings->r_float(strSection, "armor");
	if (!fIsZero(fValue))
		set_info_item(m_pArmor.get(), fValue, h);

	SetHeight(h);
}
