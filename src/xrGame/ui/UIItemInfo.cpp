#include "pch_script.h"

#include "uiiteminfo.h"
#include "uistatic.h"
#include "UIXmlInit.h"

#include "UIProgressBar.h"
#include "UIScrollView.h"
#include "UIFrameWindow.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "../string_table.h"
#include "../Inventory_Item.h"
#include "UIInventoryUtilities.h"
#include "../PhysicsShellHolder.h"
#include "UIWpnParams.h"
#include "ui_af_params.h"
#include "UIInvUpgradeProperty.h"
#include "UIOutfitInfo.h"
#include "UIBoosterInfo.h"
#include "../Weapon.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../eatable_item.h"
#include "UICellItem.h"
#include "UIActorMenu.h"
#include "uigamecustom.h"
#include "clsid_game.h"
#include "CUIAddonInfo.h"
#include "UICellCustomItems.h"

extern const LPCSTR g_inventory_upgrade_xml;

CUIItemInfo::CUIItemInfo()
{
	UIItemImageSize.set			(0.0f,0.0f);
	
	UICost						= NULL;
	UITradeTip					= NULL;
	UIWeight					= NULL;
	UIVolume					= NULL;
	UICondition					= NULL;
	UIAmount					= NULL;
	UIItemImage					= NULL;
	UIDesc						= NULL;
	UIWpnParams					= NULL;
	UIProperties				= NULL;
	UIOutfitInfo				= NULL;
	UIBoosterInfo				= NULL;
	UIArtefactParams			= NULL;
	UIName						= NULL;
	UIBackground				= NULL;
	m_b_FitToHeight				= false;
}

CUIItemInfo::~CUIItemInfo()
{
	xr_delete	(UIWpnParams);
	xr_delete	(UIArtefactParams);
	xr_delete	(UIProperties);
	xr_delete	(UIOutfitInfo);
	xr_delete	(UIBoosterInfo);
}

void CUIItemInfo::InitItemInfo(LPCSTR xml_name)
{
	CUIXml							uiXml;
	uiXml.Load						(CONFIG_PATH, UI_PATH, xml_name);
	CUIXmlInit						xml_init;

	uiXml.NavigateToNode			("main_frame", 0);
	xml_init.InitWindow				(uiXml, "main_frame", 0, this);
	delay							= uiXml.ReadAttribInt("main_frame", 0, "delay", 500);

	uiXml.NavigateToNode			("background_frame", 0);
	UIBackground					= xr_new<CUIFrameWindow>();
	UIBackground->SetAutoDelete		(true);
	AttachChild						(UIBackground);
	xml_init.InitFrameWindow		(uiXml, "background_frame", 0,	UIBackground);

	uiXml.NavigateToNode			("static_name", 0);
	UIName							= xr_new<CUITextWnd>();	 
	AttachChild						(UIName);		
	UIName->SetAutoDelete			(true);
	xml_init.InitTextWnd			(uiXml, "static_name", 0,	UIName);

	uiXml.NavigateToNode			("static_weight", 0);
	UIWeight						= xr_new<CUITextWnd>();	 
	AttachChild						(UIWeight);		
	UIWeight->SetAutoDelete			(true);
	xml_init.InitTextWnd			(uiXml, "static_weight", 0, UIWeight);

	uiXml.NavigateToNode			("static_volume", 0);
	UIVolume						= xr_new<CUITextWnd>();	 
	AttachChild						(UIVolume);		
	UIVolume->SetAutoDelete			(true);
	xml_init.InitTextWnd			(uiXml, "static_volume", 0, UIVolume);

	uiXml.NavigateToNode			("static_condition", 0);
	UICondition						= xr_new<CUITextWnd>();	 
	AttachChild						(UICondition);		
	UICondition->SetAutoDelete		(true);
	xml_init.InitTextWnd			(uiXml, "static_condition", 0, UICondition);

	uiXml.NavigateToNode			("static_amount", 0);
	UIAmount						= xr_new<CUITextWnd>();	 
	AttachChild						(UIAmount);		
	UIAmount->SetAutoDelete			(true);
	xml_init.InitTextWnd			(uiXml, "static_amount", 0, UIAmount);

	uiXml.NavigateToNode			("static_cost", 0);
	UICost							= xr_new<CUITextWnd>();	 
	AttachChild						(UICost);
	UICost->SetAutoDelete			(true);
	xml_init.InitTextWnd			(uiXml, "static_cost", 0, UICost);

	uiXml.NavigateToNode			("static_no_trade", 0);
	UITradeTip						= xr_new<CUITextWnd>();	 
	AttachChild						(UITradeTip);
	UITradeTip->SetAutoDelete		(true);
	xml_init.InitTextWnd			(uiXml, "static_no_trade", 0, UITradeTip);

	uiXml.NavigateToNode			("descr_list", 0);
	UIWpnParams						= xr_new<CUIWpnParams>();
	UIWpnParams->InitFromXml		(uiXml);

	UIArtefactParams				= xr_new<CUIArtefactParams>();
	UIArtefactParams->InitFromXml	(uiXml);

	UIBoosterInfo					= xr_new<CUIBoosterInfo>();
	UIBoosterInfo->InitFromXml		(uiXml);
	
	m_addon_info					= create_xptr<CUIAddonInfo>();
	m_addon_info->initFromXml		(uiXml);

	if (ai().get_alife())
	{
		UIProperties				= xr_new<UIInvUpgPropertiesWnd>();
		UIProperties->init_from_xml	("actor_menu_item.xml");
	}

	UIDesc							= xr_new<CUIScrollView>(); 
	AttachChild						(UIDesc);		
	UIDesc->SetAutoDelete			(true);
	m_desc_info.bShowDescrText		= !!uiXml.ReadAttribInt("descr_list",0,"only_text_info", 1);
	m_b_FitToHeight					= !!uiXml.ReadAttribInt("descr_list",0,"fit_to_height", 0);
	xml_init.InitScrollView			(uiXml, "descr_list", 0, UIDesc);
	xml_init.InitFont				(uiXml, "descr_list:font", 0, m_desc_info.uDescClr, m_desc_info.pDescFont);	

	if (uiXml.NavigateToNode("image_static", 0))
	{	
		UIItemImage					= xr_new<CUIStatic>();	 
		AttachChild					(UIItemImage);	
		UIItemImage->SetAutoDelete	(true);
		xml_init.InitStatic			(uiXml, "image_static", 0, UIItemImage);
		UIItemImage->TextureOn		();

		UIItemImage->TextureOff		();
		UIItemImageSize.set			(UIItemImage->GetWidth(),UIItemImage->GetHeight());
	}

	if (uiXml.NavigateToNode("outfit_info", 0))
	{
		UIOutfitInfo				= xr_new<CUIOutfitInfo>();
		UIOutfitInfo->InitFromXml	(uiXml);
	}

	xml_init.InitAutoStaticGroup	(uiXml, "auto", 0, this);
}

void CUIItemInfo::InitItemInfo(Fvector2 pos, Fvector2 size, LPCSTR xml_name)
{
	inherited::SetWndPos	(pos);
	inherited::SetWndSize	(size);
    InitItemInfo			(xml_name);
}

void CUIItemInfo::InitItem(CUICellItem* pCellItem, u32 item_price, LPCSTR trade_tip)
{
	if (!pCellItem)
	{
		Enable							(false);
		return;
	}

	PIItem pInvItem						= (PIItem)pCellItem->m_pData;
	shared_str& section					= pCellItem->m_section;
	Enable								(NULL != pInvItem || section != "");
	if (!pInvItem && section == "")
		return;

	string256							str;

	if (UIName)
		UIName->SetText					((pInvItem) ? pInvItem->getName() : CInventoryItem::readName(section));

	if (UIWeight)
	{
		float weight					= (pInvItem) ? pInvItem->Weight() : CurrentGameUI()->GetActorMenu().CalcItemWeight(*section);
		LPCSTR kg_str					= CStringTable().translate("st_kg").c_str();
		xr_sprintf						(str, "%3.2f %s", weight, kg_str );
		UIWeight->SetText				(str);
	}

	if (UICondition)
	{
		LPCSTR							condition_str;
		float condition					= (pInvItem) ? pInvItem->GetCondition() : 1.f;
		if (fMoreOrEqual(condition, 1.f))
			condition_str				= "st_prestine";
		else if (fMoreOrEqual(condition, 0.8f))
			condition_str				= "st_almost_prestine";
		else if (fMoreOrEqual(condition, 0.6f))
			condition_str				= "st_worn";
		else if (fMoreOrEqual(condition, 0.4f))
			condition_str				= "st_damaged";
		else if (fMoreOrEqual(condition, 0.2f))
			condition_str				= "st_badly_damaged";
		else if (fMore(condition, 0.f))
			condition_str				= "st_almost_ruined";
		else
			condition_str				= "st_ruined";

		xr_sprintf						(str, "%s: %s", *CStringTable().translate("st_condition"), *CStringTable().translate(condition_str));
		UICondition->SetText			(str);
	}

	if (UIVolume)
	{
		float volume					= (pInvItem) ? pInvItem->Volume() : CurrentGameUI()->GetActorMenu().CalcItemVolume(*section);
		LPCSTR li_str					= CStringTable().translate("st_li").c_str();
		xr_sprintf						(str, "%3.2f %s", volume, li_str);
		UIVolume->SetText				(str);
	}

	if (UICost && item_price!=u32(-1))
	{
		UICost->SetText					(CurrentGameUI()->GetActorMenu().FormatMoney(item_price));
		UICost->Show					(true);
	}
	else
		UICost->Show					(false);
	
	if (UIAmount)
	{
		shared_str amount_str			= "";
		if (pSettings->line_exist(section, "amount_display_type"))
		{
			LPCSTR amount_display_type	= pSettings->r_string(section, "amount_display_type");
			CCustomOutfit* outfit		= smart_cast<CCustomOutfit*>(pInvItem);
			CHelmet* helmet				= smart_cast<CHelmet*>(pInvItem);
			bool no_nightvision			= false;
			if (pInvItem)
			{
				if (outfit || helmet)
					no_nightvision		= outfit && !outfit->m_NightVisionSect.size() || helmet && !helmet->m_NightVisionSect.size();
			}
			else if (pCellItem->ClassID() == CLSID_EQUIPMENT_STALKER || pCellItem->ClassID() == CLSID_EQUIPMENT_HELMET)
				no_nightvision			= !pSettings->line_exist(section, "nightvision_sect");

			if (xr_strcmp(amount_display_type, "hide") && !no_nightvision)
			{
				LPCSTR title					= *CStringTable().translate(pSettings->r_string(amount_display_type, "title"));
				LPCSTR unit						= pSettings->r_string(amount_display_type, "unit");
				bool empty_cont					= READ_IF_EXISTS(pSettings, r_bool, section, "container", FALSE) && (pSettings->r_u16(section, "supplies_count") == 0);
				float amount					= (pInvItem)	? pInvItem->GetAmount()		: (empty_cont) ? 0.f : pSettings->r_float(section, "capacity");
				float fill						= (pInvItem)	? pInvItem->GetFill()		: (empty_cont) ? 0.f : 1.f;
				if (!xr_strcmp(unit, "percent"))
					amount_str.printf			("%s: %i%%", title, (u32)floor(fill * 100.f));
				else if (!xr_strcmp(unit, "fill"))
				{
					LPCSTR						fill_str;
					if (1.f-EPS < fill)
						fill_str				= "st_full";
					else if (0.8f-EPS < fill)
						fill_str				= "st_almost_full";
					else if (0.6f-EPS < fill)
						fill_str				= "st_more_than_half";
					else if (0.4f-EPS < fill)
						fill_str				= "st_around_half";
					else if (0.2f-EPS < fill)
						fill_str				= "st_less_than_half";
					else if (EPS < fill)
						fill_str				= "st_almost_empty";
					else
						fill_str				= "st_empty";
					amount_str.printf			("%s: %s", title, *CStringTable().translate(fill_str));
				}
				else
				{
					amount						*= pSettings->r_float(amount_display_type, "factor");
					LPCSTR unit_str				= *CStringTable().translate(unit);
					bool integer				= !!pSettings->r_bool(amount_display_type, "integer");
					if (!xr_strcmp(unit, "none"))
						if (integer)
							amount_str.printf	("%s: %d", title, (u32)round(amount));
						else
							amount_str.printf	("%s: %.2f", title, amount);
					else
						if (integer)
							amount_str.printf	("%s: %d %s", title, (u32)round(amount), unit_str);
						else
							amount_str.printf	("%s: %.2f %s", title, amount, unit_str);
				}
			}
		}

		UIAmount->SetText						(*amount_str);
	}
	
	if (UITradeTip)
	{
		if (!trade_tip)
			UITradeTip->Show					(false);
		else
		{
			UITradeTip->SetText					(CStringTable().translate(trade_tip).c_str());
			UITradeTip->AdjustHeightToText		();
			UITradeTip->Show					(true);
		}
	}
	
	if (UIDesc)
	{
		Fvector2 pos							= UIDesc->GetWndPos();
		if (trade_tip)
			pos.y								= UITradeTip->GetWndPos().y + UITradeTip->GetHeight() + 4.0f;

		UIDesc->SetWndPos						(pos);
		UIDesc->Clear							();
		VERIFY									(!UIDesc->GetSize());

		if (m_desc_info.bShowDescrText)
		{
			CUITextWnd* pItem					= xr_new<CUITextWnd>();
			pItem->SetTextColor					(m_desc_info.uDescClr);
			pItem->SetFont						(m_desc_info.pDescFont);
			pItem->SetWidth						(UIDesc->GetDesiredChildWidth());
			pItem->SetTextComplexMode			(true);
			pItem->SetText						(*((pInvItem) ? pInvItem->ItemDescription() : CStringTable().translate(READ_IF_EXISTS(pSettings, r_string, section, "description", ""))));
			pItem->AdjustHeightToText			();
			UIDesc->AddWindow					(pItem, true);
		}

		TryAddWpnInfo							(pCellItem);
		TryAddArtefactInfo						(pCellItem);
		TryAddOutfitInfo						(pCellItem);
		TryAddUpgradeInfo						(pCellItem);
		TryAddBoosterInfo						(pCellItem);
		tryAddAddonInfo							(pCellItem);

		if (m_b_FitToHeight)
		{
			UIDesc->SetHeight					(UIDesc->GetPadSize().y);
			float new_height					= UIDesc->GetWndPos().y + UIDesc->GetWndSize().y + UIName->GetWndPos().y;
			SetHeight							(new_height);
			if (UIBackground)
				UIBackground->SetHeight			(new_height);
		}

		UIDesc->ScrollToBegin					();
	}

	if (UIItemImage)
	{
		// Загружаем картинку
		UIItemImage->SetShader					(InventoryUtilities::GetEquipmentIconsShader(pCellItem->m_section));

		UIItemImage->GetUIStaticItem().SetTextureRect(pInvItem->GetIconRect());
		UIItemImage->TextureOn					();
		UIItemImage->SetStretchTexture			(true);

		Fvector2								v_r;
		pInvItem->GetIconRect().getsize			(v_r);
		v_r.mul									(pSettings->r_float(pCellItem->m_section, "icon_scale"));

		UIItemImage->GetUIStaticItem().SetSize	(v_r);
		UIItemImage->SetWidth					(v_r.x);
		UIItemImage->SetHeight					(v_r.y);
	}
}

void CUIItemInfo::TryAddWpnInfo(CUICellItem* itm)
{
	if (ItemCategory(itm->m_section, "weapon") && !ItemSubcategory(itm->m_section, "melee") && !ItemSubcategory(itm->m_section, "grenade"))
	{
		UIWpnParams->SetInfo		(itm);
		UIDesc->AddWindow			(UIWpnParams, false);
	}
}

void CUIItemInfo::TryAddArtefactInfo(CUICellItem* itm)
{
	if (ItemCategory(itm->m_section, "artefact"))
	{
		CArtefact* artefact			= smart_cast<CArtefact*>((PIItem)itm->m_pData);
		UIArtefactParams->SetInfo	(*itm->m_section, artefact);
		UIDesc->AddWindow			(UIArtefactParams, false);
	}
}

void CUIItemInfo::TryAddOutfitInfo(CUICellItem* itm)
{
	if (ItemCategory(itm->m_section, "outfit"))
	{
		if (ItemSubcategory(itm->m_section, "helmet"))
			UIOutfitInfo->UpdateInfoHelmet		(itm);
		else if (!ItemSubcategory(itm->m_section, "backpack"))
			UIOutfitInfo->UpdateInfoSuit		(itm);
		else
			return;
		UIDesc->AddWindow						(UIOutfitInfo, false);
	}
}

void CUIItemInfo::TryAddUpgradeInfo(CUICellItem* itm)
{
	PIItem item						= (PIItem)itm->m_pData;
	if (item && item->upgardes().size())
	{
		UIProperties->set_item_info	(item);
		UIDesc->AddWindow			(UIProperties, false);
	}
}

void CUIItemInfo::TryAddBoosterInfo(CUICellItem* itm)
{
	UIBoosterInfo->SetInfo			(itm);
	UIDesc->AddWindow				(UIBoosterInfo, false);
}

void CUIItemInfo::tryAddAddonInfo(CUICellItem* itm)
{
	if (READ_IF_EXISTS(pSettings, r_bool, itm->m_section, "addon", FALSE) || smart_cast<CUIAddonOwnerCellItem*>(itm))
	{
		m_addon_info->setInfo		(itm);
		UIDesc->AddWindow			(m_addon_info.get(), false);
	}
}

void CUIItemInfo::Draw()
{
	if (m_bIsEnabled)
		inherited::Draw();
}
