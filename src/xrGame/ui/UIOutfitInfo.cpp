#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "UIXmlInit.h"
#include "UIStatic.h"
#include "UIDoubleProgressBar.h"
#include "UICellItem.h"

#include "..\CustomOutfit.h"
#include "..\ActorHelmet.h"
#include "..\string_table.h"
#include "..\actor.h"
#include "..\ActorCondition.h"
#include "..\player_hud.h"
#include "purchase_list.h"

LPCSTR protection_sections[] =
{
	"burn_protection",
	"shock_protection",
	"chemical_burn_protection",
	"radiation_protection",
	"telepatic_protection"
};

LPCSTR protection_captions[] =
{
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection"
};

CUIOutfitProtection::CUIOutfitProtection()
{
	AttachChild		(&m_name);
	AttachChild		(&m_value);
}

CUIOutfitProtection::~CUIOutfitProtection()
{
}

void CUIOutfitProtection::InitFromXml(CUIXml& xml_doc, LPCSTR base_str, u32 type)
{
	CUIXmlInit::InitWindow					(xml_doc, base_str, 0, this);
	string256								buf;
	strconcat								(sizeof(buf), buf, base_str, ":", protection_sections[type]);
	CUIXmlInit::InitWindow					(xml_doc, buf, 0, this);
	CUIXmlInit::InitStatic					(xml_doc, buf, 0, &m_name);
	m_name.TextItemControl()->SetTextST		(protection_captions[type]);
	strconcat								(sizeof(buf), buf, base_str, ":", protection_sections[type], ":static_value");
	CUIXmlInit::InitTextWnd					(xml_doc, buf, 0, &m_value);
	m_magnitude								= xml_doc.ReadAttribFlt(buf, 0, "magnitude", 1.f);
}

void CUIOutfitProtection::SetValue(float value)
{
	shared_str			str;
	str.printf			("%.2f", value * m_magnitude);
	m_value.SetText		(*str);
}

// ===========================================================================================

CUIOutfitInfo::CUIOutfitInfo()
{
	for (u32 i = 0; i < eProtectionTypeMax; ++i)
		m_items[i] = NULL;
}

CUIOutfitInfo::~CUIOutfitInfo()
{
	for (u32 i = 0; i < eProtectionTypeMax; ++i)
		xr_delete(m_items[i]);
}

void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR base_str				= "outfit_info";
	CUIXmlInit::InitWindow		(xml_doc, base_str, 0, this);

	string128 buf;

	m_Prop_line						= xr_new<CUIStatic>();
	AttachChild						(m_Prop_line);
	m_Prop_line->SetAutoDelete		(true);
	strconcat						(sizeof(buf), buf, base_str, ":", "prop_line");
	CUIXmlInit::InitStatic			(xml_doc, buf, 0, m_Prop_line);

	m_textArmorClass					= xr_new<CUITextWnd>();
	AttachChild							(m_textArmorClass);
	m_textArmorClass->SetAutoDelete		(true);
	strconcat							(sizeof(buf), buf, base_str, ":", "armor_class");
	CUIXmlInit::InitTextWnd				(xml_doc, buf, 0, m_textArmorClass);

	m_textArmorClassValue					= xr_new<CUITextWnd>();
	AttachChild								(m_textArmorClassValue);
	m_textArmorClassValue->SetAutoDelete	(true);
	strconcat								(sizeof(buf), buf, base_str, ":", "armor_class_value");
	CUIXmlInit::InitTextWnd					(xml_doc, buf, 0, m_textArmorClassValue);

	Fvector2								pos;
	pos.set									(m_textArmorClass->GetWndPos());
	pos.y									+= m_textArmorClass->GetWndSize().y;

	//Alundaio: Specific Display Order
	m_items[eBurnProtection]				= xr_new<CUIOutfitProtection>();
	m_items[eBurnProtection]->InitFromXml	(xml_doc, base_str, eBurnProtection);
	AttachChild								(m_items[eBurnProtection]);
	m_items[eBurnProtection]->SetWndPos		(pos);
	pos.y									+= m_items[eBurnProtection]->GetWndSize().y;

	m_items[eShockProtection]				= xr_new<CUIOutfitProtection>();
	m_items[eShockProtection]->InitFromXml	(xml_doc, base_str, eShockProtection);
	AttachChild								(m_items[eShockProtection]);
	m_items[eShockProtection]->SetWndPos	(pos);
	pos.y									+= m_items[eShockProtection]->GetWndSize().y;

	m_items[eChemBurnProtection]				= xr_new<CUIOutfitProtection>();
	m_items[eChemBurnProtection]->InitFromXml	(xml_doc, base_str, eChemBurnProtection);
	AttachChild									(m_items[eChemBurnProtection]);
	m_items[eChemBurnProtection]->SetWndPos		(pos);
	pos.y										+= m_items[eChemBurnProtection]->GetWndSize().y;

	m_items[eRadiationProtection]				= xr_new<CUIOutfitProtection>();
	m_items[eRadiationProtection]->InitFromXml	(xml_doc, base_str, eRadiationProtection);
	AttachChild									(m_items[eRadiationProtection]);
	m_items[eRadiationProtection]->SetWndPos	(pos);
	pos.y										+= m_items[eRadiationProtection]->GetWndSize().y;

	m_items[eTelepaticProtection]				= xr_new<CUIOutfitProtection>();
	m_items[eTelepaticProtection]->InitFromXml	(xml_doc, base_str, eTelepaticProtection);
	AttachChild									(m_items[eTelepaticProtection]);
	m_items[eTelepaticProtection]->SetWndPos	(pos);
	pos.y										+= m_items[eTelepaticProtection]->GetWndSize().y;

	pos.x		= GetWndSize().x;
	SetWndSize	(pos);
}

void CUIOutfitInfo::UpdateInfoSuit(CUICellItem* itm)
{
	if (!smart_cast<CActor*>(Level().CurrentViewEntity()))
		return;
	
	auto descr							= CInventoryItem::readDescription(itm->m_section.c_str());
	m_Prop_line->Show					(descr && descr[0]);
	
	PIItem item							= static_cast<PIItem>(itm->m_pData);
	CCustomOutfit* outfit				= smart_cast<CCustomOutfit*>(item);

	for (u32 i = 0; i < eProtectionTypeMax; ++i)
	{
		float val						= (outfit) ? outfit->GetHitTypeProtection((ALife::EHitType)i) : pSettings->r_float(itm->m_section, protection_sections[i]);
		m_items[i]->SetValue			(val);
	}

	float armor_level					= (outfit) ? outfit->m_fArmorLevel : pSettings->r_float(itm->m_section, "armor_level");
	if (armor_level < 1.f)
	{
		m_textArmorClass->SetTextST		("ui_armor_clothes");
		m_textArmorClassValue->SetText	("");
	}
	else
	{
		m_textArmorClass->SetTextST		("ui_armor_level");
		shared_str						str;
		str.printf						("%.1f %s", armor_level, *CStringTable().translate("st_class"));
		m_textArmorClassValue->SetText	(*str);
	}
}

void CUIOutfitInfo::UpdateInfoHelmet(CUICellItem* itm)
{
	CActor* actor		= smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
		return;
	m_Prop_line->Show	(!!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, itm->m_section, "description", ""), ""));
	
	PIItem item					= (PIItem)itm->m_pData;
	CHelmet* helmet				= smart_cast<CHelmet*>(item);

	for (u32 i = 0; i < eProtectionTypeMax; ++i)
	{
		float val					= (helmet) ? helmet->GetHitTypeProtection((ALife::EHitType)i) : pSettings->r_float(itm->m_section, protection_sections[i]);
		m_items[i]->SetValue		(val);
	}

	float armor_level						= (helmet) ? helmet->m_fArmorLevel : pSettings->r_float(itm->m_section, "armor_level");
	if (armor_level < 1.f)
	{
		m_textArmorClass->SetTextST			("ui_armor_clothes");
		m_textArmorClassValue->SetText		("");
	}
	else
	{
		m_textArmorClass->SetTextST			("ui_armor_level");
		shared_str							str;
		str.printf							("%.1f %s", armor_level, *CStringTable().translate("st_class"));
		m_textArmorClassValue->SetText		(*str);
	}
}