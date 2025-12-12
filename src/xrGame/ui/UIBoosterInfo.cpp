#include "stdafx.h"
#include "UIBoosterInfo.h"

#include "UIXmlInit.h"
#include "UICellItem.h"
#include "CUIMiscInfoItem.h"

#include "string_table.h"

LPCSTR boost_influence_caption[] =
{
	"ui_inv_painkill",
	"ui_inv_regeneration",
	"ui_inv_recuperation",
	"ui_inv_endurance",
	"ui_inv_tenacity",
	"ui_inv_antirad"
};

CUIBoosterInfo::CUIBoosterInfo() = default;
CUIBoosterInfo::~CUIBoosterInfo() = default;

void CUIBoosterInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "booster_params" };

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strBaseNode, 0));

	for (u32 i = 0; i < eBoostMaxCount; ++i)
	{
		m_boosts[i]->init(xmlDoc, ef_boosters_section_names[i]);
		m_boosts[i]->SetCaption(CStringTable().translate(boost_influence_caption[i]).c_str());
	}

	m_need_hydration->init(xmlDoc, "need_hydration");
	m_need_hydration->SetCaption(CStringTable().translate("ui_inv_hydration").c_str());

	m_need_satiety->init(xmlDoc, "need_satiety");
	m_need_satiety->SetCaption(CStringTable().translate("ui_inv_satiety").c_str());

	m_health_outer->init(xmlDoc, "health_outer");
	m_health_outer->SetCaption(CStringTable().translate("ui_inv_health_outer").c_str());

	m_health_neural->init(xmlDoc, "health_neural");
	m_health_neural->SetCaption(CStringTable().translate("ui_inv_health_neural").c_str());

	m_power_short->init(xmlDoc, "power_short");
	m_power_short->SetCaption(CStringTable().translate("ui_inv_power_short").c_str());

	m_booster_anabiotic->init(xmlDoc, "anabiotic");
	m_booster_anabiotic->SetCaption(CStringTable().translate("ui_inv_survive_surge").c_str());

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
			if (fIsZero(fValue))
				continue;

			m_boosts[i]->SetValue(fValue);
			m_boosts[i]->SetY(h);
			h += m_boosts[i]->GetWndSize().y;
			AttachChild(m_boosts[i].get());
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "need_hydration"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "need_hydration");
		if (!fIsZero(fValue))
		{
			m_need_hydration->SetValue(fValue);
			m_need_hydration->SetY(h);
			h += m_need_hydration->GetWndSize().y;
			AttachChild(m_need_hydration.get());
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "need_satiety"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "need_satiety");
		if (!fIsZero(fValue))
		{
			m_need_satiety->SetValue(fValue);
			m_need_satiety->SetY(h);
			h += m_need_satiety->GetWndSize().y;
			AttachChild(m_need_satiety.get());
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "health_outer"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "health_outer");
		if (!fIsZero(fValue))
		{
			m_health_outer->SetValue(fValue);
			m_health_outer->SetY(h);
			h += m_health_outer->GetWndSize().y;
			AttachChild(m_health_outer.get());
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "health_neural"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "health_neural");
		if (!fIsZero(fValue))
		{
			m_health_neural->SetValue(fValue);
			m_health_neural->SetY(h);
			h += m_health_neural->GetWndSize().y;
			AttachChild(m_health_neural.get());
		}
	}

	if (pSettings->line_exist(pCellItem->m_section, "power_short"))
	{
		fValue = pSettings->r_float(pCellItem->m_section, "power_short");
		if (!fIsZero(fValue))
		{
			m_power_short->SetValue(fValue);
			m_power_short->SetY(h);
			h += m_power_short->GetWndSize().y;
			AttachChild(m_power_short.get());
		}
	}

	if (!xr_strcmp(pCellItem->m_section, "drug_anabiotic"))
	{
		m_booster_anabiotic->SetY(h);
		h += m_booster_anabiotic->GetWndSize().y;
		AttachChild(m_booster_anabiotic.get());
	}

	SetHeight(h);
}
