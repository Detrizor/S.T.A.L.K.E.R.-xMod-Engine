#include "stdafx.h"
#include "CUIMiscInfoItem.h"

#include "UIHelper.h"
#include "UIStatic.h"
#include "UIXmlInit.h"

#include "string_table.h"

CUIMiscInfoItem::CUIMiscInfoItem(CUIWindow* pParent) : m_pParent(pParent) {}
CUIMiscInfoItem::~CUIMiscInfoItem() = default;

void CUIMiscInfoItem::init(CUIXml& xmlDoc, LPCSTR strSection, bool bAttach)
{
	CUIXmlInit::InitWindow(xmlDoc, strSection, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strSection));

	if (bAttach)
	{
		m_pParent->AttachChild(this);
		m_bAttaching = false;
	}

	m_fMagnitude = xmlDoc.ReadAttribFlt("value", 0, "magnitude", 1.F);
	m_bShowSign = !!xmlDoc.ReadAttribInt("value", 0, "show_sign", 1);
	m_bPercUnit = !!xmlDoc.ReadAttribInt("value", 0, "perc_unit", 0);

	AttachChild(m_pIcon.get());
	CUIXmlInit::InitStatic(xmlDoc, "icon", 0, m_pIcon.get());

	AttachChild(m_pCaption.get());
	CUIXmlInit::InitTextWnd(xmlDoc, "caption", 0, m_pCaption.get());
	setCaption(m_pCaption->TextItemControl().GetText());

	AttachChild(m_pValue.get());
	CUIXmlInit::InitTextWnd(xmlDoc, "value", 0, m_pValue.get());

	LPCSTR strUnit = xmlDoc.ReadAttrib("value", 0, "unit_str", "");
	if (strUnit && strUnit[0])
		m_strUnit._set(CStringTable().translate(strUnit));

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIMiscInfoItem::setCaption(LPCSTR strCaption)
{
	std::string strCaptionTranslated{ CStringTable().translate(strCaption).c_str() };
	strCaptionTranslated += ":";
	m_pCaption->TextItemControl().SetTextST(strCaptionTranslated.c_str());
	m_pCaption->AdjustWidthToText();
}

void CUIMiscInfoItem::setValue(float fValue, float& h)
{
	fValue *= m_fMagnitude;

	shared_str str{};
	bool bFormat{ (abs(fValue - round(fValue)) < .05F) };
	str.printf((m_bShowSign) ? ((bFormat) ? "%+.0f" : "%+.1f") : ((bFormat) ? "%.0f" : "%.1f"), fValue);

	if (m_bPercUnit)
		str.printf("%s%%", str.c_str());
	else if (m_strUnit.size())
		str.printf("%s %s", str.c_str(), m_strUnit.c_str());
	setStrValue(str.c_str(), h);
}

void CUIMiscInfoItem::setStrValue(LPCSTR strValue, float& h)
{
	m_pValue->SetTextST(strValue);
	m_pValue->AdjustHeightToText();
	SetHeight(m_pValue->GetHeight());
	SetY(h);
	h += GetHeight();
	if (m_bAttaching)
		m_pParent->AttachChild(this);
}
