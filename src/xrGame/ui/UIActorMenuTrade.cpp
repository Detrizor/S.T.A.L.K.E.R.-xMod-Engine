//#include "stdafx.h"
#include "pch_script.h"
#include "UIActorMenu.h"
#include "UI3tButton.h"
#include "UIComboBox.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICharacterInfo.h"
#include "UIFrameLineWnd.h"
#include "UICellItem.h"
#include "UIInventoryUtilities.h"
#include "UITalkWnd.h"
#include "UICellCustomItems.h"

#include "../InventoryOwner.h"
#include "../Inventory.h"
#include "../Trade.h"
#include "../Entity.h"
#include "../Actor.h"
#include "../Weapon.h"
#include "../trade_parameters.h"
#include "../inventory_item_object.h"
#include "../string_table.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "../UIGameSP.h"
#include "../item_container.h"

static CUICellItem* create_cell_item_from_section(shared_str CR$ section)
{
	return								(pSettings->r_bool_ex(section, "addon_owner", false)) ?
		xr_new<CUIAddonOwnerCellItem>(section) :
		xr_new<CUIInventoryCellItem>(section);
}

// -------------------------------------------------

void CUIActorMenu::InitTradeMode()
{
	m_pInventoryBagList->Show		(false);
	m_pTrashList->Show				(false);
	m_PartnerCharacterInfo->Show	(true);
	m_PartnerMoney->Show			(true);

	m_pTradeActorBagList->Show		(false);
	m_pTradeActorList->Show			(true);
	m_pTradePartnerBagList->Show	(true);
	m_pTradePartnerList->Show		(true);
	
	m_ActorInventoryTrade->Show		(true);
	m_PartnerInventoryTrade->Show	(true);

	m_trade_buy_button->Show		(true);
	m_trade_sell_button->Show		(true);

	m_trade_list->Show				(true);

	VERIFY							( m_pPartnerInvOwner );
	m_pPartnerInvOwner->StartTrading();

	InitInventoryContents			();

	UpdatePocketsPresence();
	
	luabind::functor<LPCSTR>			funct;
	ai().script_engine().functor		("trade_manager.get_supplies", funct);
	LPCSTR sections						= funct(m_pPartnerInvOwner->object_id());
	if (sections[0])
	{
		string512						buf;
		shared_str						str;
		m_trade_list->ClearList			();
		for (int i = 0, i_e = _GetItemCount(sections); i != i_e; i++)
		{
			LPCSTR item					= _GetItem(sections, i, buf);
			str.printf					("st_%s", *CStringTable().translate(item));
			m_trade_list->AddItem_	(*str, i);
		}
		m_trade_list->SetItemIDX		(0);
		OnTradeList						(NULL, NULL);
	}
	InitPartnerInventoryContents		();

	m_actor_trade					= m_pActorInvOwner->GetTrade();
	m_partner_trade					= m_pPartnerInvOwner->GetTrade();
	VERIFY							( m_actor_trade );
	VERIFY							( m_partner_trade );
	m_actor_trade->StartTradeEx		( m_pPartnerInvOwner );
	m_partner_trade->StartTradeEx	( m_pActorInvOwner );

	init_actor_trade					();
	update_partner_trade				();
}

bool is_item_in_list(CUIDragDropListEx* pList, PIItem item)
{
	for(u16 i=0;i<pList->ItemsCount();i++)
	{
		CUICellItem* cell_item = pList->GetItemIdx(i);
		for(u16 k=0;k<cell_item->ChildsCount();k++)
		{
			CUICellItem* inv_cell_item = cell_item->Child(k);
			if((PIItem)inv_cell_item->m_pData==item)
				return true;
		}
		if((PIItem)cell_item->m_pData==item)
			return true;
	}
	return false;
}

void CUIActorMenu::InitPartnerInventoryContents()
{
	m_pTradePartnerBagList->ClearAll	(true);
	CAI_Trader* trader					= smart_cast<CAI_Trader*>(m_pPartnerInvOwner);
	if (trader)
		for (auto& str : trader->supplies_list)
			m_pTradePartnerBagList->SetItem(create_cell_item_from_section(str));
	else
	{
		TIItemContainer					items_list;
		m_pPartnerInvOwner->inventory().AddAvailableItems(items_list, true);
		_STD sort						(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);
		for (auto item : items_list)
			if (!is_item_in_list(m_pTradePartnerList, item))
				m_pTradePartnerBagList->SetItem(item->getIcon());
	}
	m_trade_partner_inventory_state		= m_pPartnerInvOwner->inventory().ModifyFrame();
}

void CUIActorMenu::ColorizeItem(CUICellItem* itm)
{
	u32 col								= (CanMoveToPartner((PIItem)itm->m_pData)) ? 255 : 100;
	itm->SetTextureColor				(color_rgba(255, col, col, 255));
}

void CUIActorMenu::DeInitTradeMode()
{
	if (m_actor_trade)
		m_actor_trade->StopTrade		();
	if (m_partner_trade)
		m_partner_trade->StopTrade		();
	if (m_pPartnerInvOwner)
		m_pPartnerInvOwner->StopTrading	();

	m_pTrashList->Show					(true);
	m_PartnerCharacterInfo->Show		(false);
	m_PartnerMoney->Show				(false);

	m_pTradeActorBagList->Show			(false);
	m_pTradeActorList->Show				(false);
	m_pTradePartnerBagList->Show		(false);
	m_pTradePartnerList->Show			(false);

	m_ActorInventoryTrade->Show			(false);
	m_PartnerInventoryTrade->Show		(false);
	
	m_trade_buy_button->Show			(false);
	m_trade_sell_button->Show			(false);
	
	m_trade_list->Show					(false);

	if (CurrentGameUI())
	{
		//только если находимся в режиме single
		CUIGameSP* pGameSP				= smart_cast<CUIGameSP*>(CurrentGameUI());
		if (pGameSP && pGameSP->TalkMenu->IsShown())
			pGameSP->TalkMenu->NeedUpdateQuestions();
	}

	if (m_pActorInv)
		while (!m_pActorInv->getTradeContainer().empty())
			m_pActorInv->Ruck			(m_pActorInv->getTradeContainer().back());
}

bool CUIActorMenu::ToActorTrade(CUICellItem* itm, bool b_use_cursor_pos)
{
	auto item							= static_cast<PIItem>(itm->m_pData);
	if (CanMoveToPartner(item))
	{
		if (item->parent_id() == m_pActorInvOwner->object_id())
			m_pActorInv->toTrade		(item);
		else
			pickup_item					(item, eItemPlaceTrade);
		return							true;
	}
	
	return								false;
}

bool CUIActorMenu::ToPartnerTrade(CUICellItem* itm, bool b_use_cursor_pos)
{
	if (m_pPartnerInvOwner->AllowItemToTrade(itm->m_section, SInvItemPlace()))
	{
		PIItem item						= static_cast<PIItem>(itm->m_pData);
		m_pTradePartnerList->SetItem	((item) ? item->getIcon() : create_cell_item_from_section(itm->m_section));
		update_partner_trade			();
		return							true;
	}
	return								false;
}

bool CUIActorMenu::ToPartnerTradeBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	CUICellItem* removed_cell			= itm->OwnerList()->RemoveItem(itm, false);
	removed_cell->destroy				();
	update_partner_trade				();
	return								true;
}

float CUIActorMenu::CalcItemWeight(LPCSTR section)
{
	float res				= pSettings->r_float(section, "inv_weight") + READ_IF_EXISTS(pSettings, r_float, section, "net_weight", 0.f);
	if (float supplies_count = READ_IF_EXISTS(pSettings, r_float, section, "supplies_count", 0.f))
		res					+= supplies_count * pSettings->r_float(pSettings->r_string(section, "supplies"), "inv_weight");
	return					res;
}

float CUIActorMenu::CalcItemsWeight(CUIDragDropListEx* pList)
{
	float res				= 0.f;
	for (u32 i = 0; i < pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		PIItem item			= (PIItem)itm->m_pData;
		res					+= (item) ? item->Weight() : CalcItemWeight(*itm->m_section);
		for (u32 j = 0; j < itm->ChildsCount(); ++j)
		{
			PIItem jitem	= (PIItem)itm->Child(j)->m_pData;
			res				+= (jitem) ? jitem->Weight() : CalcItemWeight(*itm->Child(j)->m_section);
		}
	}
	return					res;
}

float CUIActorMenu::CalcItemVolume(LPCSTR section)
{
	float res					= pSettings->r_float(section, "inv_volume") + READ_IF_EXISTS(pSettings, r_float, section, "net_volume", 0.f);
	if (READ_IF_EXISTS(pSettings, r_BOOL, section, "content_volume_scale", FALSE))
		if (float supplies_count = READ_IF_EXISTS(pSettings, r_float, section, "supplies_count", 0.f))
			res					+= supplies_count * pSettings->r_float(pSettings->r_string(section, "supplies"), "inv_volume");
	return						res;
}

float CUIActorMenu::CalcItemsVolume(CUIDragDropListEx* pList)
{
	float res				= 0.f;
	for (u32 i = 0; i < pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		PIItem	iitem		= (PIItem)itm->m_pData;
		res					+= (iitem) ? iitem->Volume() : CalcItemVolume(*itm->m_section);
		for (u32 j = 0; j < itm->ChildsCount(); ++j)
		{
			PIItem jitem	= (PIItem)itm->Child(j)->m_pData;
			res				+= (jitem) ? jitem->Volume() : CalcItemVolume(*itm->Child(j)->m_section);
		}
	}
	return					res;
}

u32 CUIActorMenu::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
	u32 res					= 0;
	for (u32 i = 0; i < pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		res					+= pTrade->GetItemPrice(itm, bBuying);
		for (u32 j = 0; j < itm->ChildsCount(); ++j)
			res				+= pTrade->GetItemPrice(itm->Child(j), bBuying);
	}
	return					res;
}

bool CUIActorMenu::CanMoveToPartner(PIItem pItem, shared_str* reason)
{
	if (m_currMenuMode != mmTrade || !m_pPartnerInvOwner)
		return true;

	if (!pItem->CanTrade() || !m_pPartnerInvOwner->trade_parameters().enabled(CTradeParameters::action_buy(0), pItem->object().cNameSect()))
	{
		if (reason)
			*reason				= "st_no_trade_tip_1";
		return					false;
	}
	
	float condition_factor		= m_pPartnerInvOwner->trade_parameters().buy_item_condition_factor;
	if (pItem->Category("weapon") || pItem->Category("outfit"))
		condition_factor		*= CInventoryItem::s_max_repair_condition;
	if (fLess(pItem->GetCondition(), condition_factor))
	{
		if (reason)
			*reason				= "st_no_trade_tip_2";
		return					false;
	}
	
	CWeaponMagazined* wpn		= smart_cast<CWeaponMagazined*>(pItem);
	if (wpn && !wpn->CanTrade())
	{
		if (reason)
			*reason				= "st_no_trade_tip_3";
		return					false;
	}
	
	return true;
}

LPCSTR CUIActorMenu::FormatMoney(u32 money)
{
	shared_str				money_str;
	
	float currate			= pSettings->r_float("miscellaneous", "currency_rate");
	LPCSTR					mask;
	int i					= 0;
	while (currate - floor(currate) > EPS)
	{
		currate				*= 10.f;
		i++;
	}
	int currate_e			= pSettings->r_s8("miscellaneous", "currency_rate_e");
	i						-= currate_e;
	mask					= (i > 0) ? *money_str.printf("%%.%df", i) : "%d";
	float fmoney			= (float)money * currate * pow(10.f, currate_e);
	float tmp				= modf(fmoney * 0.1f, &fmoney) * 10.f;
	if (i > 0)
		money_str.printf(mask, tmp);
	else
		money_str.printf(mask, (u8)round(tmp));
	i						= 0;
	float					i3;
	while (fmoney > EPS)
	{
		i++;
		tmp						= round(modf(fmoney * 0.1f, &fmoney) * 10.f);
		i3						= float(i) / 3.f;
		if (i3 - floor(i3) < EPS)
			money_str.printf	("%d%s%s", u8(tmp), money_delimiter, *money_str);
		else
			money_str.printf	("%d%s", u8(tmp), *money_str);
	}
	
	return					*money_str.printf("%s %s", *money_str, currency_str);
}

void CUIActorMenu::UpdateActor()
{
	m_ActorMoney->SetText				(FormatMoney(m_pActorInvOwner->get_money()));
}

void CUIActorMenu::UpdatePartnerBag()
{
	if (m_pPartnerInvOwner->InfinitiveMoney())
		m_PartnerMoney->SetText		("---");
	else
		m_PartnerMoney->SetText		(FormatMoney(m_pPartnerInvOwner->get_money()));
}

void CUIActorMenu::OnBtnPerformTradeBuy(CUIWindow* w, void* d)
{
	if(m_pTradePartnerList->ItemsCount()==0) 
	{
		return;
	}

	int actor_money    = (int)m_pActorInvOwner->get_money();
	int partner_money  = (int)m_pPartnerInvOwner->get_money();
	int actor_price    = 0;//(int)CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
	int partner_price  = (int)CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );

	int delta_price    = actor_price - partner_price;
	actor_money        += delta_price;
	partner_money      -= delta_price;

	if ( ( actor_money >= 0 ) /*&& ( partner_money >= 0 )*/ && ( actor_price >= 0 || partner_price > 0 ) )
	{
		m_partner_trade->OnPerformTrade( partner_price, actor_price );

//		TransferItems( m_pTradeActorList,   m_pTradePartnerBagList, m_partner_trade, true );
		TransferItems( m_pTradePartnerList,	m_pTradeActorBagList,	m_partner_trade, false );
	}
	else
	{
		if (actor_money < 0)
			CallMessageBoxOK("not_enough_money_actor");
		else
			CallMessageBoxOK("trade_dont_make");
	}
	SetCurrentItem					( NULL );

	UpdateActor();
	UpdatePartnerBag();
}

void CUIActorMenu::OnTradeList(CUIWindow* w, void* d)
{
	u32 idx							= m_trade_list->GetSelectedIDX();
	luabind::functor<void>			funct;
	ai().script_engine().functor	("trade_manager.update_supplies", funct);
	funct							(m_pPartnerInvOwner->object_id(), idx);
}

void CUIActorMenu::OnBtnPerformTradeSell(CUIWindow* w, void* d)
{
	if ( m_pTradeActorList->ItemsCount() == 0 ) 
	{
		return;
	}

	int actor_money    = (int)m_pActorInvOwner->get_money();
	int partner_money  = (int)m_pPartnerInvOwner->get_money();
	int actor_price    = (int)CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
	int partner_price  = 0;//(int)CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );

	int delta_price    = actor_price - partner_price;
	actor_money        += delta_price;
	partner_money      -= delta_price;

	if ( ( actor_money >= 0 ) && ( partner_money >= 0 ) && ( actor_price >= 0 || partner_price > 0 ) )
	{
		m_partner_trade->OnPerformTrade( partner_price, actor_price );

		TransferItems( m_pTradeActorList,   m_pTradePartnerBagList, m_partner_trade, true );
//		TransferItems( m_pTradePartnerList,	m_pTradeActorBagList,	m_partner_trade, false );
	}
	else
	{
/*		if ( actor_money < 0 )
		{
			CallMessageBoxOK( "not_enough_money_actor" );
		}
		else */if ( partner_money < 0 )
		{
			CallMessageBoxOK( "not_enough_money_partner" );
		}
		else
		{
			CallMessageBoxOK( "trade_dont_make" );
		}
	}
	SetCurrentItem					( NULL );

	UpdateActor();
	UpdatePartnerBag();
}

void CUIActorMenu::TransferItems(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying)
{
	bool trader								= !!smart_cast<CAI_Trader*>(m_pPartnerInvOwner);
	while (pSellList->ItemsCount())
	{
		CUICellItem* cell_item				= pSellList->RemoveItem(pSellList->GetItemIdx(0), false);
		pTrade->TransferItem				(cell_item, bBuying);
		if (trader && !bBuying)
			cell_item->destroy				();
		else
			pBuyList->SetItem				(cell_item);
	}
	pTrade->pThis.inv_owner->set_money		(pTrade->pThis.inv_owner->get_money(), true);
	pTrade->pPartner.inv_owner->set_money	(pTrade->pPartner.inv_owner->get_money(), true);
}

//Alundaio: Donate current item while in trade menu
void CUIActorMenu::DonateCurrentItem(CUICellItem* cell_item)
{
	if (!m_partner_trade || !m_pTradePartnerList)
		return;

	CUIDragDropListEx* invlist = GetListByType(iActorBag);
	if (!invlist->IsOwner(cell_item))
		return;

	PIItem item = (PIItem)cell_item->m_pData;
	if (!item)
		return;

	//Alundaio: 
	luabind::functor<bool> funct;
	if (ai().script_engine().functor("actor_menu_inventory.CUIActorMenu_DonateCurrentItem", funct))
	{
		if (funct(m_pPartnerInvOwner->cast_game_object()->lua_game_object(), item->object().lua_game_object()) == false)
			return;
	}
	//-Alundaio

	CUICellItem* itm = invlist->RemoveItem(cell_item, false);

	m_partner_trade->TransferItem(cell_item, true, true);

	m_pTradePartnerList->SetItem(itm);

	SetCurrentItem(NULL);
}
//-Alundaio

void CUIActorMenu::init_actor_trade()
{
	TIItemContainer items_list			= m_pActorInv->getTradeContainer();
	_STD sort							(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);

	for (auto& item : items_list)
	{
		CUICellItem* itm				= item->getIcon();
		m_pTradeActorList->SetItem		(itm);
		ColorizeItem					(itm);
	}

	u32 actor_price						= CalcItemsPrice(m_pTradeActorList, m_partner_trade, true);
	m_ActorTradePrice->SetText			(FormatMoney(actor_price));
	m_ActorTradePrice->AdjustWidthToText();

	float actor_weight					 = CalcItemsWeight(m_pTradeActorList);
	float actor_volume					= CalcItemsVolume(m_pTradeActorList);

	string64							buf;
	LPCSTR kg_str						= CStringTable().translate("st_kg").c_str();
	xr_sprintf							(buf, "%.1f %s", actor_weight, kg_str);
	m_ActorTradeWeight->SetText			(buf);
	
	LPCSTR li_str						= CStringTable().translate("st_li").c_str();
	xr_sprintf							(buf, "%.1f %s", actor_volume, li_str);
	m_ActorTradeVolume->SetText			(buf);

	m_pTradeActorList->setValid			(true);
}

void CUIActorMenu::update_partner_trade()
{
	u32 partner_price					= CalcItemsPrice(m_pTradePartnerList, m_partner_trade, false);
	m_PartnerTradePrice->SetText		(FormatMoney(partner_price));
	m_PartnerTradePrice->AdjustWidthToText();

	float partner_weight				= CalcItemsWeight(m_pTradePartnerList);
	float partner_volume				= CalcItemsVolume(m_pTradePartnerList);

	string64							buf;
	LPCSTR kg_str						= CStringTable().translate("st_kg").c_str();
	xr_sprintf							(buf, "%.1f %s", partner_weight, kg_str);
	m_PartnerTradeWeight->SetText		(buf);
	
	LPCSTR li_str						= CStringTable().translate("st_li").c_str();
	xr_sprintf							(buf, "%.1f %s", partner_volume, li_str);
	m_PartnerTradeVolume->SetText		(buf);
}
