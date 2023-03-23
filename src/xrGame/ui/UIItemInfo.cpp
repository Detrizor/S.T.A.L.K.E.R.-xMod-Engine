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

extern const LPCSTR g_inventory_upgrade_xml;

#define  INV_GRID_WIDTH2  40.0f
#define  INV_GRID_HEIGHT2 40.0f

CUIItemInfo::CUIItemInfo()
{
	UIItemImageSize.set			(0.0f,0.0f);
	
	UICost						= NULL;
	UITradeTip					= NULL;
	UIWeight					= NULL;
	UIVolume					= NULL;
	UICondition					= NULL;
	UIItemImage					= NULL;
	UIDesc						= NULL;
//	UIConditionWnd				= NULL;
	UIWpnParams					= NULL;
	UIProperties				= NULL;
	UIOutfitInfo				= NULL;
	UIBoosterInfo				= NULL;
	UIArtefactParams			= NULL;
	UIName						= NULL;
	UIBackground				= NULL;
	m_b_FitToHeight				= false;
	m_complex_desc				= false;
}

CUIItemInfo::~CUIItemInfo()
{
//	xr_delete	(UIConditionWnd);
	xr_delete	(UIWpnParams);
	xr_delete	(UIArtefactParams);
	xr_delete	(UIProperties);
	xr_delete	(UIOutfitInfo);
	xr_delete	(UIBoosterInfo);
}

void CUIItemInfo::InitItemInfo(LPCSTR xml_name)
{
	CUIXml						uiXml;
	uiXml.Load					(CONFIG_PATH, UI_PATH, xml_name);
	CUIXmlInit					xml_init;

	if(uiXml.NavigateToNode("main_frame",0))
	{
		Frect wnd_rect;
		wnd_rect.x1		= uiXml.ReadAttribFlt("main_frame", 0, "x", 0);
		wnd_rect.y1		= uiXml.ReadAttribFlt("main_frame", 0, "y", 0);

		wnd_rect.x2		= uiXml.ReadAttribFlt("main_frame", 0, "width", 0);
		wnd_rect.y2		= uiXml.ReadAttribFlt("main_frame", 0, "height", 0);
		wnd_rect.x2		+= wnd_rect.x1;
		wnd_rect.y2		+= wnd_rect.y1;
		inherited::SetWndRect(wnd_rect);
		
		delay			= uiXml.ReadAttribInt("main_frame", 0, "delay", 500);
	}
	if(uiXml.NavigateToNode("background_frame",0))
	{
		UIBackground				= xr_new<CUIFrameWindow>();
		UIBackground->SetAutoDelete	(true);
		AttachChild					(UIBackground);
		xml_init.InitFrameWindow	(uiXml, "background_frame", 0,	UIBackground);
	}
	m_complex_desc = false;
	if(uiXml.NavigateToNode("static_name",0))
	{
		UIName						= xr_new<CUITextWnd>();	 
		AttachChild					(UIName);		
		UIName->SetAutoDelete		(true);
		xml_init.InitTextWnd		(uiXml, "static_name", 0,	UIName);
		m_complex_desc				= ( uiXml.ReadAttribInt("static_name", 0, "complex_desc", 0) == 1 );
	}
	if(uiXml.NavigateToNode("static_weight",0))
	{
		UIWeight				= xr_new<CUITextWnd>();	 
		AttachChild				(UIWeight);		
		UIWeight->SetAutoDelete(true);
		xml_init.InitTextWnd		(uiXml, "static_weight", 0,			UIWeight);
	}
	if(uiXml.NavigateToNode("static_volume",0))
	{
		UIVolume				= xr_new<CUITextWnd>();	 
		AttachChild				(UIVolume);		
		UIVolume->SetAutoDelete(true);
		xml_init.InitTextWnd		(uiXml, "static_volume", 0,			UIVolume);
	}
	if (uiXml.NavigateToNode("static_condition", 0))
	{
		UICondition					= xr_new<CUITextWnd>();	 
		AttachChild					(UICondition);		
		UICondition->SetAutoDelete	(true);
		xml_init.InitTextWnd		(uiXml, "static_condition", 0, UICondition);
	}

	if(uiXml.NavigateToNode("static_cost",0))
	{
		UICost					= xr_new<CUITextWnd>();	 
		AttachChild				(UICost);
		UICost->SetAutoDelete	(true);
		xml_init.InitTextWnd		(uiXml, "static_cost", 0,			UICost);
	}

	if(uiXml.NavigateToNode("static_no_trade",0))
	{
		UITradeTip					= xr_new<CUITextWnd>();	 
		AttachChild					(UITradeTip);
		UITradeTip->SetAutoDelete	(true);
		xml_init.InitTextWnd		(uiXml, "static_no_trade", 0,		UITradeTip);
	}

	if(uiXml.NavigateToNode("descr_list",0))
	{
//		UIConditionWnd					= xr_new<CUIConditionParams>();
//		UIConditionWnd->InitFromXml		(uiXml);
		UIWpnParams						= xr_new<CUIWpnParams>();
		UIWpnParams->InitFromXml		(uiXml);

		UIArtefactParams				= xr_new<CUIArtefactParams>();
		UIArtefactParams->InitFromXml	(uiXml);

		UIBoosterInfo					= xr_new<CUIBoosterInfo>();
		UIBoosterInfo->InitFromXml		(uiXml);

		//UIDesc_line						= xr_new<CUIStatic>();
		//AttachChild						(UIDesc_line);	
		//UIDesc_line->SetAutoDelete		(true);
		//xml_init.InitStatic				(uiXml, "description_line", 0, UIDesc_line);

		if ( ai().get_alife() ) // (-designer)
		{
			UIProperties					= xr_new<UIInvUpgPropertiesWnd>();
			UIProperties->init_from_xml		("actor_menu_item.xml");
		}

		UIDesc							= xr_new<CUIScrollView>(); 
		AttachChild						(UIDesc);		
		UIDesc->SetAutoDelete			(true);
		m_desc_info.bShowDescrText		= !!uiXml.ReadAttribInt("descr_list",0,"only_text_info", 1);
		m_b_FitToHeight					= !!uiXml.ReadAttribInt("descr_list",0,"fit_to_height", 0);
		xml_init.InitScrollView			(uiXml, "descr_list", 0, UIDesc);
		xml_init.InitFont				(uiXml, "descr_list:font", 0, m_desc_info.uDescClr, m_desc_info.pDescFont);
	}	

	if (uiXml.NavigateToNode("image_static", 0))
	{	
		UIItemImage					= xr_new<CUIStatic>();	 
		AttachChild					(UIItemImage);	
		UIItemImage->SetAutoDelete	(true);
		xml_init.InitStatic			(uiXml, "image_static", 0, UIItemImage);
		UIItemImage->TextureOn		();

		UIItemImage->TextureOff			();
		UIItemImageSize.set				(UIItemImage->GetWidth(),UIItemImage->GetHeight());
	}
	if ( uiXml.NavigateToNode( "outfit_info", 0 ) )
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
		Enable				(false);
		return;
	}

	PIItem pInvItem			= (PIItem)pCellItem->m_pData;
	shared_str& section		= pCellItem->m_section;
	Enable					(NULL != pInvItem || section != "");
	if (!pInvItem && section == "")
		return;

	Fvector2				pos;	pos.set( 0.0f, 0.0f );
	string256				str;
	if ( UIName )
	{
		UIName->SetText(*CStringTable().translate(pSettings->r_string(section, "inv_name")));
		UIName->AdjustHeightToText();
		pos.y = UIName->GetWndPos().y + UIName->GetHeight() + 4.0f;
	}
	if ( UIWeight )
	{
		LPCSTR  kg_str = CStringTable().translate("st_kg").c_str();
		float	weight = (pInvItem) ? pInvItem->Weight() : pSettings->r_float(section, "inv_weight");
		
		if (!weight)
		{
			if (CWeaponAmmo* ammo = dynamic_cast<CWeaponAmmo*>(pInvItem))
			{
				// its helper item, m_boxCur is zero, so recalculate via CInventoryItem::Weight()
				weight = pInvItem->CInventoryItem::Weight();
				for( u32 j = 0; j < pCellItem->ChildsCount(); ++j )
				{
					PIItem jitem	= (PIItem)pCellItem->Child(j)->m_pData;
					weight			+= jitem->CInventoryItem::Weight();
				}
			}
		}

		xr_sprintf				(str, "%3.2f %s", weight, kg_str );
		UIWeight->SetText	(str);
		UIWeight->AdjustWidthToText();
		
		pos.x = UIWeight->GetWndPos().x;
		if (m_complex_desc)
			UIWeight->SetWndPos	(pos);
	}
	if (UIVolume)
	{
		LPCSTR  li_str = CStringTable().translate("st_li").c_str();
		float	volume = (pInvItem) ? pInvItem->Volume() : pSettings->r_float(section, "inv_volume");

		if (!volume)
		{
			if (CWeaponAmmo* ammo = dynamic_cast<CWeaponAmmo*>(pInvItem))
			{
				// its helper item, m_boxCur is zero, so recalculate via CInventoryItem::Weight()
				volume = pInvItem->CInventoryItem::Volume();
				for (u32 j = 0; j < pCellItem->ChildsCount(); ++j)
				{
					PIItem jitem = (PIItem)pCellItem->Child(j)->m_pData;
					volume += jitem->CInventoryItem::Volume();
				}

			}
		}

		xr_sprintf			(str, "%3.2f %s", volume, li_str);
		UIVolume->SetText	(str);

		UIVolume->AdjustWidthToText	();
		pos.x						= UIWeight->GetWndPos().x + UIWeight->GetWidth() + 3.f;
		UIVolume->SetWndPos			(pos);
	}
	if (UICondition)
	{
		shared_str				cond;
		CEatableItem* eitm		= smart_cast<CEatableItem*>(pInvItem);
		float condition			= (pInvItem) ? pInvItem->GetCondition() : 1.f;
		if (eitm && eitm->DiscreteCondition() || READ_IF_EXISTS(pSettings, r_bool, section, "discrete_condition", false))
		{
			u8 max_uses			= (eitm) ? eitm->GetMaxUses() : READ_IF_EXISTS(pSettings, r_u8, section, "max_uses", 1);
			u8 uses				= (eitm) ? eitm->GetRemainingUses() : max_uses;
			if (uses > 0)
				cond.printf		("%i/%i", uses, max_uses);
			else
				cond			= CStringTable().translate("st_empty");
		}
		else if (pInvItem && pInvItem->PercentCondition())
		{
			if (condition > EPS || !eitm)
				cond.printf		("%i%%", 10 * (u8)roundf(condition * 10.f));
			else
				cond			= CStringTable().translate("st_empty");
		}
		else if (condition > 0.8f)
			cond				= CStringTable().translate("st_prestine");
		else if (condition > 0.6f)
			cond				= CStringTable().translate("st_worn");
		else if (condition > 0.4f)
			cond				= CStringTable().translate("st_damaged");
		else if (condition > 0.2f)
			cond				= CStringTable().translate("st_badly_damaged");
		else
			cond				= CStringTable().translate("st_ruined");
		
		LPCSTR cond_str			= CStringTable().translate("st_cond").c_str();
		xr_sprintf				(str, "%s %s", cond_str, *cond);
		UICondition->SetText	(str);

		UICondition->AdjustWidthToText	();
		pos.x							= GetWidth() - UICondition->GetWidth() - UIWeight->GetWndPos().x;
		UICondition->SetWndPos			(pos);
	}
	if ( UICost && item_price!=u32(-1) )
	{
		LPCSTR cur_str			= CStringTable().translate("st_currency").c_str();
		xr_sprintf				(str, "%d %s", item_price, cur_str);// will be owerwritten in multiplayer
		UICost->SetText			(str);
		
		UICost->AdjustWidthToText		();
		pos.x							= 0.5f * (GetWidth() - UICost->GetWidth());
		UICost->SetWndPos				(pos);
		pos.x							= UIWeight->GetWndPos().x;
		UICost->Show					(true);
	}
	else
		UICost->Show(false);
	
	if ( UITradeTip)
	{
		pos.y = UITradeTip->GetWndPos().y;
		if (m_complex_desc)
		{
			if (UIWeight)
				pos.y = UIWeight->GetWndPos().y + UIWeight->GetHeight() + 4.0f;
			if (UIVolume)
				pos.y = UIVolume->GetWndPos().y + UIVolume->GetHeight() + 4.0f;
		}

		if(trade_tip==NULL)
			UITradeTip->Show(false);
		else
		{
			UITradeTip->SetText(CStringTable().translate(trade_tip).c_str());
			UITradeTip->AdjustHeightToText();
			UITradeTip->SetWndPos(pos);
			UITradeTip->Show(true);
		}
	}
	
	if (UIDesc)
	{
		pos = UIDesc->GetWndPos();
		if ( UIWeight )
			pos.y = UIWeight->GetWndPos().y + UIWeight->GetHeight() + 4.0f;
		if (UIVolume)
			pos.y = UIVolume->GetWndPos().y + UIVolume->GetHeight() + 4.0f;

		if(UITradeTip && trade_tip!=NULL)
			pos.y = UITradeTip->GetWndPos().y + UITradeTip->GetHeight() + 4.0f;

		UIDesc->SetWndPos		(pos);
		UIDesc->Clear			();
		VERIFY					(0==UIDesc->GetSize());
		if(m_desc_info.bShowDescrText)
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
		TryAddConditionInfo					(pCellItem);
		TryAddWpnInfo						(pCellItem);
		TryAddArtefactInfo					(pCellItem);
		TryAddOutfitInfo					(pCellItem);
		TryAddUpgradeInfo					(pCellItem);
		TryAddBoosterInfo					(pCellItem);

		if(m_b_FitToHeight)
		{
			UIDesc->SetWndSize				(Fvector2().set(UIDesc->GetWndSize().x, UIDesc->GetPadSize().y) );
			Fvector2 new_size;
			new_size.x						= GetWndSize().x;
			new_size.y						= UIDesc->GetWndPos().y+UIDesc->GetWndSize().y+20.0f;
			new_size.x						= _max(105.0f, new_size.x);
			new_size.y						= _max(105.0f, new_size.y);
			
			SetWndSize						(new_size);
			if(UIBackground)
				UIBackground->SetWndSize	(new_size);
		}

		UIDesc->ScrollToBegin				();
	}
	if(UIItemImage)
	{
		// Загружаем картинку
		UIItemImage->SetShader				(InventoryUtilities::GetEquipmentIconsShader());

		Irect item_grid_rect				= pInvItem->GetInvGridRect();
		Frect texture_rect;
		texture_rect.lt.set					(item_grid_rect.x1*INV_GRID_WIDTH,	item_grid_rect.y1*INV_GRID_HEIGHT);
		texture_rect.rb.set					(item_grid_rect.x2*INV_GRID_WIDTH,	item_grid_rect.y2*INV_GRID_HEIGHT);
		texture_rect.rb.add					(texture_rect.lt);
		UIItemImage->GetUIStaticItem().SetTextureRect(texture_rect);
		UIItemImage->TextureOn				();
		UIItemImage->SetStretchTexture		(true);
		Fvector2 v_r						= { item_grid_rect.x2*INV_GRID_WIDTH2,	
												item_grid_rect.y2*INV_GRID_HEIGHT2};
		
		v_r.x								*= UI().get_current_kx();

		UIItemImage->GetUIStaticItem().SetSize	(v_r);
		UIItemImage->SetWidth					(v_r.x);
		UIItemImage->SetHeight					(v_r.y);
	}
}

void CUIItemInfo::TryAddConditionInfo(CUICellItem* item)
{
}

void CUIItemInfo::TryAddWpnInfo(CUICellItem* itm)
{
	LPCSTR subclass					= READ_IF_EXISTS(pSettings, r_string, itm->m_section, "subclass", "nil");
	if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, itm->m_section, "main_class", "nil"), "weapon") && xr_strcmp(subclass, "melee") && xr_strcmp(subclass, "grenade"))
	{
		UIWpnParams->SetInfo		(itm);
		UIDesc->AddWindow			(UIWpnParams, false);
	}
}

void CUIItemInfo::TryAddArtefactInfo(CUICellItem* itm)
{
	PIItem item = (PIItem)itm->m_pData;
	if (!item || item->GetCondition() == 1.f)
		return;

	if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, itm->m_section, "main_class", "nil"), "artefact"))
	{
		UIArtefactParams->SetInfo	(itm);
		UIDesc->AddWindow			(UIArtefactParams, false);
	}
}

void CUIItemInfo::TryAddOutfitInfo(CUICellItem* itm)
{
	if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, itm->m_section, "main_class", "nil"), "outfit"))
	{
		LPCSTR subclass							= READ_IF_EXISTS(pSettings, r_string, itm->m_section, "subclass", "nil");
		if (!xr_strcmp(subclass, "helmet"))
			UIOutfitInfo->UpdateInfoHelmet		(itm);
		else if (xr_strcmp(subclass, "backpack"))
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

void CUIItemInfo::Draw()
{
	if (m_bIsEnabled)
		inherited::Draw();
}