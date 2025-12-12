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

LPCSTR afAbsorbationNames[] = // EImmunityTypes
{
	"burn_absorbation",
	"shock_absorbation",
	"chemical_burn_absorbation",
	"radiation_absorbation",
	"telepatic_absorbation"
};

static const LPCSTR afAbsorbationCaptions[] =  // EImmunityTypes
{
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
};

static const LPCSTR afRestoreSectionNames[] = // EConditionRestoreTypes
{
	"radiation_speed",
	"painkill_speed",
	"regeneration_speed",
	"recuperation_speed"
};

static const LPCSTR afRestoreCaption[] =  // EConditionRestoreTypes
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
		for (size_t i = 0; i < eRestoreTypeMax; ++i)
		{
			float fValue{ pSettings->r_float(sec, afRestoreSectionNames[i]) };
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

	m_radiation->init(xmlDoc, "radiation");
	m_radiation->SetCaption(CStringTable().translate("st_radiation").c_str());

	for (size_t i = 0; i < eAbsorbationTypeMax; ++i)
	{
		m_absorbation_item[i]->init(xmlDoc, afAbsorbationNames[i]);
		m_absorbation_item[i]->SetCaption(CStringTable().translate(afAbsorbationCaptions[i]).c_str());
	}

	for (size_t i = 0; i < eRestoreTypeMax; ++i)
	{
		m_restore_item[i]->init(xmlDoc, afRestoreSectionNames[i]);
		m_restore_item[i]->SetCaption(CStringTable().translate(afRestoreCaption[i]).c_str());
	}

	m_weight_dump->init(xmlDoc, "weight_dump");
	m_weight_dump->SetCaption(CStringTable().translate("st_weight_dump").c_str());

	m_armor->init(xmlDoc, "armor");
	m_armor->SetCaption(CStringTable().translate("st_armor").c_str());

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIArtefactParams::set_info_item(CUIMiscInfoItem* item, float value, float& h)
{
	item->SetValue(value);
	item->SetY(h);
	h += item->GetWndSize().y;
	AttachChild(item);
}

void CUIArtefactParams::setInfo(LPCSTR section, CArtefact* art)
{
	DetachAll();
	float fValue;
	float h{ 0.F };

	fValue = (art) ? art->getRadiation() : pSettings->r_float(section, "radiation");
	if (!fIsZero(fValue))
		set_info_item(m_radiation.get(), fValue, h);

	for (size_t i = 0; i < eAbsorbationTypeMax; ++i)
	{
		fValue = (art) ? art->getAbsorbation(i) : pSettings->r_float(section, afAbsorbationNames[i]);
		if (!fIsZero(fValue))
			set_info_item(m_absorbation_item[i].get(), fValue, h);
	}

	for (size_t i = 0; i < eRestoreTypeMax; ++i)
	{
		fValue = pSettings->r_float(section, afRestoreSectionNames[i]) * ((art) ? art->getPower() : 1.F);
		if (fIsZero(fValue))
			continue;

		if (maxRestores[i] == flt_min)
			InitMaxArtValues();
		if (i != eRadiationSpeed)
			fValue /= maxRestores[i];

		set_info_item(m_restore_item[i].get(), fValue, h);
	}

	fValue = (art) ? art->getWeightDump() : pSettings->r_float(section, "weight_dump");
	if (!fIsZero(fValue))
		set_info_item(m_weight_dump.get(), fValue, h);

	fValue = (art) ? art->getArmor() : pSettings->r_float(section, "armor");
	if (!fIsZero(fValue))
		set_info_item(m_armor.get(), fValue, h);

	SetHeight(h);
}
