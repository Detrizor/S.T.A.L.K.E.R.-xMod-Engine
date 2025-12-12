#include "stdafx.h"
#include "CUIMiscInfoItem.h"

#include "UIHelper.h"
#include "UIStatic.h"
#include "UIXmlInit.h"

#include "string_table.h"

void CUIMiscInfoItem::init(CUIXml& xmlDoc, LPCSTR strSection)
{
	CUIXmlInit::InitWindow(xmlDoc, strSection, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strSection));

	m_fMagnitude = xmlDoc.ReadAttribFlt("value", 0, "magnitude", 1.F);
	m_bShowSign = !!xmlDoc.ReadAttribInt("value", 0, "show_sign", 1);
	m_bPercUnit = !!xmlDoc.ReadAttribInt("value", 0, "perc_unit", 0);

	m_pCaption = UIHelper::CreateStatic(xmlDoc, "caption", this);
	m_pValue = UIHelper::CreateTextWnd(xmlDoc, "value", this);

	LPCSTR strUnit = xmlDoc.ReadAttrib("value", 0, "unit_str", "");
	if (strUnit && strUnit[0])
		m_strUnit._set(CStringTable().translate(strUnit));

	auto strTextureMinus{ xmlDoc.Read("texture_minus", 0, "") };
	if (strTextureMinus && strTextureMinus[0])
	{
		m_strTextureMinus._set(strTextureMinus);
		auto strTexturePlus{ xmlDoc.Read("caption:texture", 0, "") };
		VERIFY(strTexturePlus && strTexturePlus[0]);
		m_strTexturePlus._set(strTexturePlus);
	}

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIMiscInfoItem::SetCaption(LPCSTR strCaption)
{
	m_pCaption->TextItemControl()->SetText(strCaption);
}

void CUIMiscInfoItem::SetValue(float fValue)
{
	fValue *= m_fMagnitude;

	shared_str str{};
	bool bFormat{ (abs(fValue - round(fValue)) < .05F) };
	str.printf((m_bShowSign) ? ((bFormat) ? "%+.0f" : "%+.1f") : ((bFormat) ? "%.0f" : "%.1f"), fValue);

	if (m_bPercUnit)
		str.printf("%s%%", str.c_str());
	else if (m_strUnit.size())
		str.printf("%s %s", str.c_str(), m_strUnit.c_str());
	m_pValue->SetText(str.c_str());

	m_pValue->SetTextColor(color_rgba(170, 170, 170, 255));
	if (m_strTextureMinus.size())
		m_pCaption->InitTexture(fMoreOrEqual(fValue, 0.F) ? m_strTexturePlus.c_str() : m_strTextureMinus.c_str());
}

void CUIMiscInfoItem::SetStrValue(LPCSTR strValue)
{
	m_pValue->SetText(strValue);
}
