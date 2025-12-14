#include "stdafx.h"
#include "UIBoosterInfo.h"

#include "UIXmlInit.h"
#include "UICellItem.h"
#include "CUIMiscInfoItem.h"

#include "string_table.h"

CUIBoosterInfo::CUIBoosterInfo() = default;
CUIBoosterInfo::~CUIBoosterInfo() = default;

void CUIBoosterInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "booster_params" };

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strBaseNode, 0));

	for (size_t i = 0; i < eBoostMaxCount; ++i)
		m_pBoosts[i]->init(xmlDoc, ef_boosters_section_names[i], false);

	m_pNeedHydration->init(xmlDoc, "need_hydration", false);
	m_pNeedSatiety->init(xmlDoc, "need_satiety", false);
	m_pHealthOuter->init(xmlDoc, "health_outer", false);
	m_pHealthNeural->init(xmlDoc, "health_neural", false);
	m_pPowerShort->init(xmlDoc, "power_short", false);
	m_pBoosterAnabiotic->init(xmlDoc, "anabiotic", false);

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIBoosterInfo::setInfo(CUICellItem* pCellItem)
{
	DetachAll();
	float fValue;
	float h{ 0.F };

	for (size_t i = 0; i < eBoostMaxCount; ++i)
	{
		if (pSettings->line_exist(pCellItem->m_section, ef_boosters_section_names[i]))
		{
			fValue = pSettings->r_float(pCellItem->m_section, ef_boosters_section_names[i]);
			if (!fIsZero(fValue))
				m_pBoosts[i]->setValue(fValue, h);
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "need_hydration"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "need_hydration");
		if (!fIsZero(fValue))
			m_pNeedHydration->setValue(fValue, h);
	}

	if (pSettings->line_exist(pCellItem->m_section, "need_satiety"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "need_satiety");
		if (!fIsZero(fValue))
			m_pNeedSatiety->setValue(fValue, h);
	}

	if (pSettings->line_exist(pCellItem->m_section, "health_outer"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "health_outer");
		if (!fIsZero(fValue))
			m_pHealthOuter->setValue(fValue, h);
	}

	if (pSettings->line_exist(pCellItem->m_section, "health_neural"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "health_neural");
		if (!fIsZero(fValue))
			m_pHealthNeural->setValue(fValue, h);
	}

	if (pSettings->line_exist(pCellItem->m_section, "power_short"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "power_short");
		if (!fIsZero(fValue))
			m_pPowerShort->setValue(fValue, h);
	}

	if (!xr_strcmp(pCellItem->m_section, "drug_anabiotic"))
		m_pBoosterAnabiotic->setStrValue("", h);

	SetHeight(h);
}
