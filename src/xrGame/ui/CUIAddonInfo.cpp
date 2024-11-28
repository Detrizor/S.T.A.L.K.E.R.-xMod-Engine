#include "stdafx.h"
#include "CUIAddonInfo.h"
#include "UIXmlInit.h"
#include "inventory_item.h"
#include "addon_owner.h"
#include "UICellCustomItems.h"

void CUIAddonInfo::initFromXml(CUIXml& xml_doc)
{
	R_ASSERT							(xml_doc.NavigateToNode("addon_info", 0));

	CUIXmlInit::InitWindow				(xml_doc, "addon_info",								0, this);
	CUIXmlInit::InitStatic				(xml_doc, "addon_info:compatible_slots_cap",		0, &m_compatible_slots_cap);
	CUIXmlInit::InitTextWnd				(xml_doc, "addon_info:compatible_slots_value",		0, &m_compatible_slots_value);
	CUIXmlInit::InitStatic				(xml_doc, "addon_info:available_slots_cap",			0, &m_available_slots_cap);
	CUIXmlInit::InitTextWnd				(xml_doc, "addon_info:available_slots_value",		0, &m_available_slots_value);
	
	AttachChild							(&m_compatible_slots_cap);
	AttachChild							(&m_compatible_slots_value);
	AttachChild							(&m_available_slots_cap);
	AttachChild							(&m_available_slots_value);
}

void CUIAddonInfo::setInfo(CUICellItem* itm)
{
	float h								= 5.f;

	bool addon							= pSettings->r_bool_ex(itm->m_section, "addon", false);
	m_compatible_slots_cap.SetVisible	(addon);
	m_compatible_slots_value.SetVisible	(addon);
	if (addon)
	{
		auto slot_type					= pSettings->r_string(itm->m_section, "slot_type");
		if (slot_type && slot_type[0])
		{
			m_compatible_slots_cap.SetY	(h);
			m_compatible_slots_value.SetY(h);
			m_compatible_slots_value.SetText(CAddonSlot::getSlotName(slot_type));
			m_compatible_slots_value.AdjustHeightToText();
			h							+= m_compatible_slots_value.GetHeight();
		}
	}

	auto uiao							= smart_cast<CUIAddonOwnerCellItem*>(itm);
	m_available_slots_cap.SetVisible	(!!uiao);
	m_available_slots_value.SetVisible	(!!uiao);
	if (uiao)
	{
		shared_str str					= "";
		for (auto& slot : uiao->Slots())
		{
			if (str.size())
				str.printf				("%s\\n", *str);
			LPCSTR color				= (slot->addon_section.size()) ? "ui_2" : "ui_4";
			str.printf					("%s%%c[%s]• %s", *str, color, *slot->name);
		}
		
		m_available_slots_cap.SetY		(h);
		m_available_slots_value.SetY	(h);
		m_available_slots_value.SetText	(*str);
		m_available_slots_value.AdjustHeightToText();
		h								+= m_available_slots_value.GetHeight();
	}

	SetHeight							(h);
}
