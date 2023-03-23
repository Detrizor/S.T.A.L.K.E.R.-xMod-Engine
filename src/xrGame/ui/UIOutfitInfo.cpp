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

float m_max_protections[]			= { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
float m_max_protections_helm[]		= { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

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
	m_levels								= (u8)xml_doc.ReadAttribInt(buf, 0, "levels", 1);
}

void CUIOutfitProtection::SetValue(float value)
{
	shared_str			str;
	str.printf			("%.1f", value * (float)m_levels);
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

	Fvector2	pos;
	pos.set		(0.0f, m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y);

	m_textArmorClass					= xr_new<CUITextWnd>();
	AttachChild							(m_textArmorClass);
	m_textArmorClass->SetAutoDelete		(true);
	strconcat							(sizeof(buf), buf, base_str, ":", "armor_class");
	CUIXmlInit::InitTextWnd				(xml_doc, buf, 0, m_textArmorClass);
	m_textArmorClass->SetWndPos			(pos);
	pos.x								+= m_textArmorClass->GetWndSize().x;

	m_textArmorClassValue					= xr_new<CUITextWnd>();
	AttachChild								(m_textArmorClassValue);
	m_textArmorClassValue->SetAutoDelete	(true);
	strconcat								(sizeof(buf), buf, base_str, ":", "armor_class_value");
	CUIXmlInit::InitTextWnd					(xml_doc, buf, 0, m_textArmorClassValue);
	m_textArmorClassValue->SetWndPos		(pos);
	pos.x									= m_textArmorClass->GetWndPos().x;
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

void InitMaxProtValues()
{
	extern CItems										ITEMS;
	MAINCLASS main_class								= ITEMS.Get("outfit");
	for (SUBCLASS subclass = main_class->second.begin(), subclass_e = main_class->second.end(); subclass != subclass_e; ++subclass)
	{
		if (subclass->first == "backpack")
			continue;
		if (subclass->first == "helmet")
		{
			for (DIVISION division = subclass->second.begin(), division_e = subclass->second.end(); division != division_e; ++division)
			{
				for (SECTION section = division->second.begin(), section_e = division->second.end(); section != section_e; ++section)
				{
					for (u8 i = 0; i < eProtectionTypeMax; ++i)
					{
						float val						= pSettings->r_float(*section, protection_sections[i]);
						if (val > m_max_protections_helm[i])
							m_max_protections_helm[i]	= val;
					}
				}
			}
		}
		else
		{
			for (DIVISION division = subclass->second.begin(), division_e = subclass->second.end(); division != division_e; ++division)
			{
				for (SECTION section = division->second.begin(), section_e = division->second.end(); section != section_e; ++section)
				{
					for (u8 i = 0; i < eProtectionTypeMax; ++i)
					{
						float val						= pSettings->r_float(*section, protection_sections[i]);
						if (val > m_max_protections[i])
							m_max_protections[i]		= val;
					}
				}
			}
		}
	}
}

void CUIOutfitInfo::UpdateInfoSuit(CUICellItem* itm)
{
	CActor* actor			= smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
		return;
	m_Prop_line->Show		(!!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, itm->m_section, "description", ""), ""));
	
	PIItem item					= (PIItem)itm->m_pData;
	CCustomOutfit* outfit		= smart_cast<CCustomOutfit*>(item);

	for (u32 i = 0; i < eProtectionTypeMax; ++i)
	{
		float val					= (outfit) ? outfit->GetHitTypeProtection((ALife::EHitType)i) : pSettings->r_float(itm->m_section, protection_sections[i]);
		if (m_max_protections[i] == 0.f)
			InitMaxProtValues		();
		val							/= m_max_protections[i];
		m_items[i]->SetValue		(val);
	}

	float armor_level						= (outfit) ? outfit->m_fArmorLevel : pSettings->r_float(itm->m_section, "armor_level");
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
		if (m_max_protections_helm[i] == 0.f)
			InitMaxProtValues		();
		val							/= m_max_protections_helm[i];
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