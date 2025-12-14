#include "stdafx.h"
#include "CUIAddonInfo.h"

#include "UIXmlInit.h"
#include "CUIMiscInfoItem.h"
#include "UICellCustomItems.h"

#include "addon.h"
#include "addon_owner.h"

CUIAddonInfo::CUIAddonInfo() = default;
CUIAddonInfo::~CUIAddonInfo() = default;

void CUIAddonInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "addon_info" };
	auto pBaseNode{ xmlDoc.NavigateToNode(strBaseNode, 0) };
	R_ASSERT(pBaseNode);

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(pBaseNode);

	m_pCompatibleSlots->init(xmlDoc, "compatible_slots", false);
	m_pAvailableSlots->init(xmlDoc, "available_slots", false);

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIAddonInfo::setInfo(CUICellItem* itm)
{
	DetachAll();
	auto pItem{ itm->getItem() };
	float h{ 0.F };

	auto addon{ (pItem) ? pItem->O.mcast<MAddon>() : nullptr };
	if (addon || pSettings->r_bool_ex(itm->m_section, "addon", false))
	{
		auto strSlotType{ (addon) ? addon->SlotType() : pSettings->r_string(itm->m_section, "slot_type") };
		if (strSlotType.size())
			m_pCompatibleSlots->setStrValue(CAddonSlot::getSlotName(strSlotType.c_str()), h);
	}

	if (auto pAOCellItem{ smart_cast<CUIAddonOwnerCellItem*>(itm) })
	{
		shared_str str{ "" };
		for (auto const& slot : pAOCellItem->Slots())
		{
			if (str.size())
				str.printf("%s\\n", str.c_str());
			auto color{ (slot->addon_section.size()) ? "ui_2" : "ui_4" };
			str.printf("%s%%c[%s]• %s", str.c_str(), color, slot->name.c_str());
		}

		m_pAvailableSlots->setStrValue(str.c_str(), h);
	}

	SetHeight(h);
}
