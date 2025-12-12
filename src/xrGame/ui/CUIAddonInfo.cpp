#include "stdafx.h"
#include "CUIAddonInfo.h"

#include "UIXmlInit.h"
#include "UICellCustomItems.h"

#include "inventory_item.h"
#include "addon_owner.h"

void CUIAddonInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "addon_info" };
	auto pBaseNode{ xmlDoc.NavigateToNode(strBaseNode, 0) };
	R_ASSERT(pBaseNode);

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(pBaseNode);

	AttachChild(&m_compatibleSlotsCap);
	CUIXmlInit::InitStatic(xmlDoc, "compatible_slots_cap", 0, &m_compatibleSlotsCap);

	AttachChild(&m_compatibleSlotsValue);
	CUIXmlInit::InitTextWnd(xmlDoc, "compatible_slots_value", 0, &m_compatibleSlotsValue);

	AttachChild(&m_availableSlotsCap);
	CUIXmlInit::InitStatic(xmlDoc, "available_slots_cap", 0, &m_availableSlotsCap);

	AttachChild(&m_availableSlotsValue);
	CUIXmlInit::InitTextWnd(xmlDoc, "available_slots_value", 0, &m_availableSlotsValue);

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIAddonInfo::setInfo(CUICellItem* itm)
{
	float h{ 5.F };

	bool bAddon{ pSettings->r_bool_ex(itm->m_section, "addon", false) };
	m_compatibleSlotsCap.SetVisible(bAddon);
	m_compatibleSlotsValue.SetVisible(bAddon);
	if (bAddon)
	{
		auto strSlotType{ pSettings->r_string(itm->m_section, "slot_type") };
		if (strSlotType && strSlotType[0])
		{
			m_compatibleSlotsCap.SetY(h);
			m_compatibleSlotsValue.SetY(h);
			m_compatibleSlotsValue.SetText(CAddonSlot::getSlotName(strSlotType));
			m_compatibleSlotsValue.AdjustHeightToText();
			h += m_compatibleSlotsValue.GetHeight();
		}
	}

	auto pAOCellItem{ smart_cast<CUIAddonOwnerCellItem*>(itm) };
	m_availableSlotsCap.SetVisible(!!pAOCellItem);
	m_availableSlotsValue.SetVisible(!!pAOCellItem);
	if (pAOCellItem)
	{
		shared_str str{ "" };
		for (auto const& slot : pAOCellItem->Slots())
		{
			if (str.size())
				str.printf("%s\\n", str.c_str());
			auto color{ (slot->addon_section.size()) ? "ui_2" : "ui_4" };
			str.printf("%s%%c[%s]• %s", str.c_str(), color, slot->name.c_str());
		}

		m_availableSlotsCap.SetY(h);
		m_availableSlotsValue.SetY(h);
		m_availableSlotsValue.SetText(*str);
		m_availableSlotsValue.AdjustHeightToText();
		h += m_availableSlotsValue.GetHeight();
	}

	SetHeight(h);
}
