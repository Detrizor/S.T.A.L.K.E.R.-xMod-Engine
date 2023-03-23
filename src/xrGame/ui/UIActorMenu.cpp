#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIActorStateInfo.h"
#include "../actor.h"
#include "../uigamesp.h"
#include "../inventory.h"
#include "../inventory_item.h"
#include "../InventoryBox.h"
#include "object_broker.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "UIInventoryUtilities.h"
#include "game_cl_base.h"

#include "../Weapon.h"
#include "../WeaponMagazinedWGrenade.h"
#include "../WeaponAmmo.h"
#include "../Silencer.h"
#include "../Scope.h"
#include "../GrenadeLauncher.h"
#include "../trade_parameters.h"
#include "../ActorHelmet.h"
#include "../CustomOutfit.h"
#include "../CustomDetector.h"
#include "../eatable_item.h"
#include "../WeaponBinoculars.h"
#include "../WeaponKnife.h"
#include "../WeaponPistol.h"

#include "UIProgressBar.h"
#include "UICursor.h"
#include "UICellItem.h"
#include "UICharacterInfo.h"
#include "UIItemInfo.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UIInventoryUpgradeWnd.h"
#include "UI3tButton.h"
#include "UIBtnHint.h"
#include "UIMessageBoxEx.h"
#include "UIPropertiesBox.h"
#include "UIMainIngameWnd.h"
#include "../Trade.h"

void CUIActorMenu::SetActor(CInventoryOwner* io)
{
	R_ASSERT			(!IsShown());
	m_last_time			= Device.dwTimeGlobal;
	m_pActorInvOwner	= io;
	
	if ( io )
		m_ActorCharacterInfo->InitCharacter	(m_pActorInvOwner->object_id());
	else
		m_ActorCharacterInfo->ClearInfo();
}

void CUIActorMenu::SetPartner(CInventoryOwner* io)
{
	R_ASSERT			(!IsShown());
	m_pPartnerInvOwner	= io;
	if ( m_pPartnerInvOwner )
	{
		if (m_pPartnerInvOwner->use_simplified_visual() ) 
			m_PartnerCharacterInfo->ClearInfo();
		else 
			m_PartnerCharacterInfo->InitCharacter( m_pPartnerInvOwner->object_id() );

		SetInvBox( NULL );
	}else
		m_PartnerCharacterInfo->ClearInfo();
}

void CUIActorMenu::SetInvBox(CInventoryBox* box)
{
	R_ASSERT			(!IsShown());
	m_pInvBox = box;
	if ( box )
	{
		m_pInvBox->set_in_use( true );
		SetPartner( NULL );
	}
}

void CUIActorMenu::SetMenuMode(EMenuMode mode)
{
	SetCurrentItem( NULL );
	m_hint_wnd->set_text( NULL );
	
	if ( mode != m_currMenuMode )
	{
		switch(m_currMenuMode)
		{
		case mmUndefined:
			break;
		case mmInventory:
			DeInitInventoryMode();
			break;
		case mmTrade:
			DeInitTradeMode();
			break;
		case mmUpgrade:
			DeInitUpgradeMode();
			break;
		case mmDeadBodySearch:
			DeInitDeadBodySearchMode();
			break;
		default:
			R_ASSERT(0);
			break;
		}

		CurrentGameUI()->UIMainIngameWnd->ShowZoneMap(false);

		m_currMenuMode = mode;
		switch(mode)
		{
		case mmUndefined:
#ifdef DEBUG
			Msg("* now is Undefined mode");
#endif // #ifdef DEBUG
			ResetMode();
			break;
		case mmInventory:
			InitInventoryMode();
#ifdef DEBUG
			Msg("* now is Inventory mode");
#endif // #ifdef DEBUG
			break;
		case mmTrade:
			InitTradeMode();
#ifdef DEBUG
			Msg("* now is Trade mode");
#endif // #ifdef DEBUG
			break;
		case mmUpgrade:
			InitUpgradeMode();
#ifdef DEBUG
			Msg("* now is Upgrade mode");
#endif // #ifdef DEBUG
			break;
		case mmDeadBodySearch:
			InitDeadBodySearchMode();
#ifdef DEBUG
			Msg("* now is DeadBodySearch mode");
#endif // #ifdef DEBUG
			break;
		default:
			R_ASSERT(0);
			break;
		}
		UpdateConditionProgressBars();
		CurModeToScript();
	}//if

	if ( m_pActorInvOwner )
	{
		UpdateOutfit();
		UpdateActor();
	}
	UpdateButtonsLayout();
}

void CUIActorMenu::PlaySnd(eActorMenuSndAction a)
{
	if (sounds[a]._handle())
        sounds[a].play					(NULL, sm_2D);
}

void CUIActorMenu::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent		(pWnd, msg, pData);
}

void CUIActorMenu::Show(bool status)
{
	inherited::Show							(status);
	SetMenuMode								((status) ? m_currMenuMode : mmUndefined);
}

void CUIActorMenu::Draw()
{
	CurrentGameUI()->UIMainIngameWnd->DrawZoneMap();
	CurrentGameUI()->UIMainIngameWnd->DrawMainIndicatorsForInventory();

	inherited::Draw	();
	m_ItemInfo->Draw();
	m_hint_wnd->Draw();
}

void CUIActorMenu::Update()
{	
	m_last_time = Device.dwTimeGlobal;

	switch ( m_currMenuMode )
	{
	case mmUndefined:
		break;
	case mmInventory:
		{
//			m_clock_value->TextItemControl()->SetText( InventoryUtilities::GetGameTimeAsString( InventoryUtilities::etpTimeToMinutes ).c_str() );
			CurrentGameUI()->UIMainIngameWnd->UpdateZoneMap();
			break;
		}
	case mmTrade:
		{
			if (m_pPartnerInvOwner->inventory().ModifyFrame() != m_trade_partner_inventory_state)
				InitPartnerInventoryContents	();
			CheckDistance						();
			break;
		}
	case mmUpgrade:
		{
			UpdateUpgradeItem();
			CheckDistance();
			break;
		}
	case mmDeadBodySearch:
		{
			//CheckDistance();
			break;
		}
	default: R_ASSERT(0); break;
	}
	
	inherited::Update();
	if (m_ItemInfo->IsEnabled())
		m_ItemInfo->Update();
	m_hint_wnd->Update();
}

bool CUIActorMenu::StopAnyMove()  // true = актёр не идёт при открытом меню
{
	switch ( m_currMenuMode )
	{
	case mmInventory:
		return false;
	case mmUndefined:
	case mmTrade:
	case mmUpgrade:
	case mmDeadBodySearch:
		return true;
	}
	return true;
}

void CUIActorMenu::CheckDistance()
{
	CGameObject* pActorGO	= smart_cast<CGameObject*>(m_pActorInvOwner);
	CGameObject* pPartnerGO	= smart_cast<CGameObject*>(m_pPartnerInvOwner);
	CGameObject* pBoxGO		= smart_cast<CGameObject*>(m_pInvBox);
	VERIFY( pActorGO && (pPartnerGO || pBoxGO) );

	if ( pPartnerGO )
	{
		if ( ( pActorGO->Position().distance_to( pPartnerGO->Position() ) > 3.0f ) &&
			!m_pPartnerInvOwner->NeedOsoznanieMode() )
		{
			g_btnHint->Discard();
			HideDialog();
		}
	}
	else //pBoxGO
	{
		VERIFY( pBoxGO );
		if ( pActorGO->Position().distance_to( pBoxGO->Position() ) > 3.0f )
		{
			g_btnHint->Discard();
			HideDialog();
		}
	}
}

EDDListType CUIActorMenu::GetListType(CUIDragDropListEx* l)
{
	if(l==m_pInventoryBagList)			return iActorBag;

	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvList[i] && m_pInvList[i] == l)
			return iActorSlot;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		if (m_pInvPocket[i] == l)
			return		iActorPocket;
	}

	if(l==m_pTradeActorBagList)			return iActorBag;
	if(l==m_pTradeActorList)			return iActorTrade;
	if(l==m_pTradePartnerBagList)		return iPartnerTradeBag;
	if(l==m_pTradePartnerList)			return iPartnerTrade;
	if(l==m_pDeadBodyBagList)			return iDeadBodyBag;

	if(l==m_pTrashList)					return iTrashSlot;

	R_ASSERT(0);
	
	return iInvalid;
}

CUIDragDropListEx* CUIActorMenu::GetListByType(EDDListType t)
{
	switch(t)
	{
		case iActorBag:
			{
				if(m_currMenuMode==mmTrade)
					return m_pTradeActorBagList;
				else
					return m_pInventoryBagList;
			}break;
		case iDeadBodyBag:
			{
				return m_pDeadBodyBagList;
			}break;
		default:
			{
				R_ASSERT("invalid call");
			}break;
	}
	return NULL;
}

CUICellItem* CUIActorMenu::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIActorMenu::CurrentIItem()
{
	return	(m_pCurrentCellItem)? (PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIActorMenu::SetCurrentItem(CUICellItem* itm)
{
	m_repair_mode = false;
	m_pCurrentCellItem = itm;
	if ( !itm )
	{
		InfoCurItem( NULL );
	}
	TryHidePropertiesBox();

	if ( m_currMenuMode == mmUpgrade )
	{
		SetupUpgradeItem();
	}
}

void CUIActorMenu::InfoCurItem( CUICellItem* cell_item )
{
	if (!cell_item)
	{
		m_ItemInfo->InitItem	(NULL);
		return;
	}

	PIItem current_item					= (PIItem)cell_item->m_pData;
	if (GetMenuMode() == mmTrade)
	{
		CInventoryOwner* item_owner		= (current_item) ? smart_cast<CInventoryOwner*>(current_item->m_pInventory->GetOwner()) : NULL;
		bool actors						= (item_owner == m_pActorInvOwner);
		u32 item_price					= m_partner_trade->GetItemPrice(cell_item, actors);

		float condition_factor			= m_pPartnerInvOwner->trade_parameters().buy_item_condition_factor;
		if (current_item && current_item->m_main_class == "weapon")
			clamp						(condition_factor, 0.f, 0.799f);
		if (actors && (!(current_item && current_item->CanTrade()) || (!m_pPartnerInvOwner->trade_parameters().enabled(CTradeParameters::action_buy(0), cell_item->m_section))))
			m_ItemInfo->InitItem		(cell_item, u32(-1), "st_no_trade_tip_1");
		else if (actors && current_item && current_item->GetCondition() < condition_factor && current_item->m_main_class != "magazine")
			m_ItemInfo->InitItem		(cell_item, u32(-1), "st_no_trade_tip_2");
		else if (current_item && smart_cast<CWeapon*>(current_item) && !smart_cast<CWeapon*>(current_item)->CanTrade())
			m_ItemInfo->InitItem		(cell_item, u32(-1), "st_no_trade_tip_3");
		else
			m_ItemInfo->InitItem		(cell_item, item_price);
	}
	else
		m_ItemInfo->InitItem			(cell_item, u32(-1));

	fit_in_rect							(m_ItemInfo, Frect().set( 0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT ), 10.f, 0.f);
}

void CUIActorMenu::UpdateItemsPlace()
{
	switch (m_currMenuMode)
	{
	case mmUndefined:
	case mmInventory:
		break;
	case mmTrade:
		UpdatePrices();
		break;
	case mmUpgrade:
		SetupUpgradeItem();
		break;
	case mmDeadBodySearch:
		UpdateDeadBodyBag();
		break;
	default:
		R_ASSERT(0);
		break;
	}

	if (m_pActorInvOwner)
	{
		UpdateOutfit();
		UpdateActor();
	}
}

// ================================================================

void CUIActorMenu::clear_highlight_lists()
{
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvSlotHighlight[i])
			m_pInvSlotHighlight[i]->Show(false);
	}

	m_pInventoryBagList->clear_select_armament();

	switch ( m_currMenuMode )
	{
	case mmUndefined:
		break;
	case mmInventory:
		break;
	case mmTrade:
		m_pTradeActorBagList->clear_select_armament();
		m_pTradeActorList->clear_select_armament();
		m_pTradePartnerBagList->clear_select_armament();
		m_pTradePartnerList->clear_select_armament();
		break;
	case mmUpgrade:
		break;
	case mmDeadBodySearch:
		m_pDeadBodyBagList->clear_select_armament();
		break;
	}
	m_highlight_clear = true;
}
void CUIActorMenu::highlight_item_slot(CUICellItem* cell_item)
{
	PIItem item = (PIItem)cell_item->m_pData;
	if(!item)
		return;

	if(CUIDragDropListEx::m_drag_item)
		return;
	
	u16 slot_id = item->BaseSlot();
	if (slot_id == PRIMARY_SLOT || slot_id == SECONDARY_SLOT)
	{ 
		if (m_pInvSlotHighlight[PRIMARY_SLOT])
			m_pInvSlotHighlight[PRIMARY_SLOT]->Show(true);

		if (m_pInvSlotHighlight[SECONDARY_SLOT])
			m_pInvSlotHighlight[SECONDARY_SLOT]->Show(true);
		return;
	}

	if (m_pInvSlotHighlight[slot_id])
	{
		m_pInvSlotHighlight[slot_id]->Show(true);
		return;
	}
}

void CUIActorMenu::set_highlight_item(CUICellItem* cell_item)
{
	highlight_item_slot(cell_item);

	switch (m_currMenuMode)
	{
	case mmUndefined:
	case mmInventory:
	case mmUpgrade:
		highlight_armament(cell_item, m_pInventoryBagList);
		break;
	case mmTrade:
		highlight_armament(cell_item, m_pTradeActorBagList);
		highlight_armament(cell_item, m_pTradeActorList);
		highlight_armament(cell_item, m_pTradePartnerBagList);
		highlight_armament(cell_item, m_pTradePartnerList);
		break;
	case mmDeadBodySearch:
		highlight_armament(cell_item, m_pInventoryBagList);
		highlight_armament(cell_item, m_pDeadBodyBagList);
		break;
	}
	m_highlight_clear = false;
}

void CUIActorMenu::highlight_armament(CUICellItem* cell_item, CUIDragDropListEx* ddlist)
{
	ddlist->clear_select_armament();
	highlight_ammo_for_weapon(cell_item, ddlist);
	highlight_weapons_for_ammo(cell_item, ddlist);
	highlight_weapons_for_addon(cell_item, ddlist);
}

extern void				FillVector(xr_vector<shared_str>&, LPCSTR, LPCSTR);
extern shared_str		FullClass(LPCSTR, bool);

void CUIActorMenu::highlight_ammo_for_weapon(CUICellItem* cell_item, CUIDragDropListEx* ddlist)
{
	VERIFY					(cell_item);
	VERIFY					(ddlist);
	shared_str subclass		= READ_IF_EXISTS(pSettings, r_string, cell_item->m_section, "subclass", "nil");
	if (xr_strcmp(READ_IF_EXISTS(pSettings, r_string, cell_item->m_section, "main_class", "nil"), "weapon") || subclass == "melee" || subclass == "grenade")
		return;

	PIItem itm						= (PIItem)cell_item->m_pData;
	CWeapon* wpn					= smart_cast<CWeapon*>(itm);
	xr_vector<shared_str>			ammo_types;
	if (wpn)
		ammo_types					= wpn->m_ammoTypes;
	else
		FillVector					(ammo_types, *cell_item->m_section, "ammo_class");

	xr_vector<shared_str>::iterator ite			= ammo_types.end();
	for (u32 i = 0, cnt = ddlist->ItemsCount(); i < cnt; ++i)
	{
		CUICellItem* ci							= ddlist->GetItemIdx(i);
		shared_str main_class					= READ_IF_EXISTS(pSettings, r_string, ci->m_section, "main_class", "nil");
		if (main_class != "ammo")
		{
			if (main_class == "addon" || main_class == "magazine")
				highlight_addons_for_weapon		(cell_item, ci);
			continue;
		}
		shared_str subclass						= READ_IF_EXISTS(pSettings, r_string, ci->m_section, "subclass", "nil");
		shared_str ammo_name					= (subclass == "box") ? pSettings->r_string(ci->m_section, "ammo_section") : ci->m_section;

		for (xr_vector<shared_str>::iterator itb = ammo_types.begin(); itb != ite; ++itb)
		{
			if (ammo_name == *itb)
			{
				ci->m_select_armament			= true;
				break;
			}
		}
	}
}

void CUIActorMenu::highlight_addons_for_weapon(CUICellItem* weapon_cell_item, CUICellItem* ci)
{
	PIItem weapon_item				= (PIItem)weapon_cell_item->m_pData;
	CWeapon* weapon					= smart_cast<CWeapon*>(weapon_item);
	shared_str& weapon_section		= weapon_cell_item->m_section;
	shared_str& addon_section		= ci->m_section;
	shared_str addon_subclass		= READ_IF_EXISTS(pSettings, r_string, addon_section, "subclass", "nil");

	u8 scope_status								= (weapon) ? (u8)weapon->get_ScopeStatus() : pSettings->r_u8(weapon_section, "scope_status");
	u8 silencer_status							= (weapon) ? (u8)weapon->get_SilencerStatus() : pSettings->r_u8(weapon_section, "silencer_status");
	u8 glauncher_status							= (weapon) ? (u8)weapon->get_GrenadeLauncherStatus() : pSettings->r_u8(weapon_section, "grenade_launcher_status");
	if (scope_status == 2 && addon_subclass == "scope")
	{
		xr_vector<shared_str>					scopes;
		if (weapon)
			scopes								= weapon->m_scopes;
		else
			FillVector							(scopes, *weapon_section, "scopes");
		if (scopes.size())
		{
			for (u32 i = 0, i_e = scopes.size(); i < i_e; i++)
			{
				if (addon_section == scopes[i])
				{
					ci->m_select_armament = true;
					break;
				}
			}
		}
	}
	else if (silencer_status == 2 && addon_subclass == "silencer")
	{
		LPCSTR silencer							= (weapon) ? *weapon->GetSilencerName() : READ_IF_EXISTS(pSettings, r_string, weapon_section, "silencer_name", 0);
		if (silencer && addon_section == silencer)
			ci->m_select_armament				= true;
	}
	else if (glauncher_status == 2 && addon_subclass == "glauncher")
	{
		LPCSTR glauncher						= (weapon) ? *weapon->GetGrenadeLauncherName() : READ_IF_EXISTS(pSettings, r_string, weapon_section, "grenade_launcher_name", 0);
		if (glauncher && addon_section == glauncher)
			ci->m_select_armament				= true;
	}
	else if (!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, addon_section, "main_class", "nil"), "magazine"))
	{
		xr_vector<shared_str>					magazines;
		if (weapon)
			magazines							= weapon->m_magazineTypes;
		else
			FillVector							(magazines, *weapon_section, "magazine_class");
		if (magazines.size())
		{
			for (u32 i = 0, i_e = magazines.size(); i < i_e; i++)
			{
				if (addon_section == magazines[i])
				{
					ci->m_select_armament		= true;
					break;
				}
			}
		}
	}
}

void CUIActorMenu::highlight_weapons_for_ammo(CUICellItem* ammo_cell_item, CUIDragDropListEx* ddlist)
{
	VERIFY		(ammo_cell_item);
	VERIFY		(ddlist);
	if (xr_strcmp(READ_IF_EXISTS(pSettings, r_string, ammo_cell_item->m_section, "main_class", "nil"), "ammo"))
		return;
	
	shared_str subclass						= READ_IF_EXISTS(pSettings, r_string, ammo_cell_item->m_section, "subclass", "nil");
	shared_str ammo_name					= (subclass == "box") ? pSettings->r_string(ammo_cell_item->m_section, "ammo_section") : ammo_cell_item->m_section;
	for (u32 i = 0, cnt = ddlist->ItemsCount(); i < cnt; ++i)
	{
		CUICellItem* ci						= ddlist->GetItemIdx(i);
		if (!pSettings->line_exist(ci->m_section, "ammo_class"))
			continue;
		PIItem item							= (PIItem)ci->m_pData;
		CWeapon* wpn						= smart_cast<CWeapon*>(item);
		xr_vector<shared_str>				ammo_types;
		if (wpn)
			ammo_types						= wpn->m_ammoTypes;
		else
			FillVector						(ammo_types, *ci->m_section, "ammo_class");

		for (xr_vector<shared_str>::iterator itb = ammo_types.begin(), ite = ammo_types.end(); itb != ite; ++itb)
		{
			if (ammo_name == *itb)
			{
				ci->m_select_armament		= true;
				break;
			}
		}
	}
}

void CUIActorMenu::highlight_weapons_for_addon(CUICellItem* addon_cell_item, CUIDragDropListEx* ddlist)
{
	VERIFY							(addon_cell_item);
	VERIFY							(ddlist);
	shared_str& addon_section		= addon_cell_item->m_section;
	shared_str addon_main_class		= READ_IF_EXISTS(pSettings, r_string, addon_section, "main_class", "nil");
	if (addon_main_class != "addon" && addon_main_class != "magazine")
		return;

	shared_str addon_subclass			= READ_IF_EXISTS(pSettings, r_string, addon_section, "subclass", "nil");
	for (u32 i = 0, cnt = ddlist->ItemsCount(); i < cnt; ++i)
	{
		CUICellItem* ci					= ddlist->GetItemIdx(i);
		if (!pSettings->line_exist(ci->m_section, "scope_status"))
			continue;
		PIItem item						= (PIItem)ci->m_pData;
		CWeapon* weapon					= smart_cast<CWeapon*>(item);
		shared_str& weapon_section		= ci->m_section;
		
		u8 scope_status								= (weapon) ? (u8)weapon->get_ScopeStatus() : pSettings->r_u8(weapon_section, "scope_status");
		u8 silencer_status							= (weapon) ? (u8)weapon->get_SilencerStatus() : pSettings->r_u8(weapon_section, "silencer_status");
		u8 glauncher_status							= (weapon) ? (u8)weapon->get_GrenadeLauncherStatus() : pSettings->r_u8(weapon_section, "grenade_launcher_status");
		if (scope_status == 2 && addon_subclass == "scope")
		{
			xr_vector<shared_str>					scopes;
			if (weapon)
				scopes								= weapon->m_scopes;
			else
				FillVector							(scopes, *weapon_section, "scopes");
			if (scopes.size())
			{
				for (u32 i = 0, i_e = scopes.size(); i < i_e; i++)
				{
					if (addon_section == scopes[i])
					{
						ci->m_select_armament		= true;
						break;
					}
				}
			}
		}
		else if (silencer_status == 2 && addon_subclass == "silencer")
		{
			LPCSTR silencer							= (weapon) ? *weapon->GetSilencerName() : READ_IF_EXISTS(pSettings, r_string, weapon_section, "silencer_name", 0);
			if (silencer && addon_section == silencer)
				ci->m_select_armament				= true;
		}
		else if (glauncher_status == 2 && addon_subclass == "glauncher")
		{
			LPCSTR glauncher						= (weapon) ? *weapon->GetGrenadeLauncherName() : READ_IF_EXISTS(pSettings, r_string, weapon_section, "grenade_launcher_name", 0);
			if (glauncher && addon_section == glauncher)
				ci->m_select_armament				= true;
		}
		else if (addon_main_class == "magazine")
		{
			xr_vector<shared_str>					magazines;
			if (weapon)
				magazines							= weapon->m_magazineTypes;
			else
				FillVector							(magazines, *weapon_section, "magazine_class");
			if (magazines.size())
			{
				for (u32 i = 0, i_e = magazines.size(); i < i_e; i++)
				{
					if (addon_section == magazines[i])
					{
						ci->m_select_armament		= true;
						break;
					}
				}
			}
		}
	}
}

// -------------------------------------------------------------------

void CUIActorMenu::ClearAllLists()
{
	m_pInventoryBagList->ClearAll				(true);

	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvList[i])
			m_pInvList[i]->ClearAll(true);
	}

	for (u8 i = 0; i < m_pockets_count; i++)
		m_pInvPocket[i]->ClearAll		(true);

	m_pTradeActorBagList->ClearAll				(true);
	m_pTradeActorList->ClearAll					(true);
	m_pTradePartnerBagList->ClearAll			(true);
	m_pTradePartnerList->ClearAll				(true);
	m_pDeadBodyBagList->ClearAll				(true);
	m_pTrashList->ClearAll						(true);
}

void CUIActorMenu::CallMessageBoxYesNo( LPCSTR text )
{
	m_message_box_yes_no->SetText( text );
	m_message_box_yes_no->func_on_ok = CUIWndCallback::void_function( this, &CUIActorMenu::OnMesBoxYes );
	m_message_box_yes_no->func_on_no = CUIWndCallback::void_function( this, &CUIActorMenu::OnMesBoxNo );
	m_message_box_yes_no->ShowDialog(false);
}

void CUIActorMenu::CallMessageBoxOK( LPCSTR text )
{
	m_message_box_ok->SetText( text );
	m_message_box_ok->ShowDialog(false);
}

void CUIActorMenu::ResetMode()
{
	ClearAllLists				();
	m_pMouseCapturer			= NULL;
	m_UIPropertiesBox->Hide		();
	SetCurrentItem				(NULL);
}

bool CUIActorMenu::CanSetItemToList(PIItem item, CUIDragDropListEx* l, u16& ret_slot)
{
	u16 item_slot		= item->BaseSlot();
	if (GetSlotList(item_slot) == l)
	{
		ret_slot		= item_slot;
		return			true;
	}

	if ((item_slot == PRIMARY_SLOT) && (l == m_pInvList[SECONDARY_SLOT]))
	{
		ret_slot		= SECONDARY_SLOT;
		return			true;
	}

	if ((item_slot == SECONDARY_SLOT) && (l == m_pInvList[PRIMARY_SLOT]))
	{
		ret_slot		= PRIMARY_SLOT;
		return			true;
	}

	u16 hand_slot		= item->HandSlot();
	if (GetSlotList(hand_slot) == l)
	{
		ret_slot		= hand_slot;
		return			true;
	}

	return false;
}
void CUIActorMenu::UpdateConditionProgressBars()
{
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		PIItem itm = m_pActorInvOwner->inventory().ItemFromSlot(i);
		if (m_pInvSlotProgress[i])
			m_pInvSlotProgress[i]->SetProgressPos(itm?iCeil(itm->GetCondition()*10.f)/10.f:0);
	}

	//Highlight 'equipped' items in actor bag
	CUIDragDropListEx* slot_list = m_pInventoryBagList;
	u32 const cnt = slot_list->ItemsCount();
	for (u32 i = 0; i < cnt; ++i)
	{
		CUICellItem* ci = slot_list->GetItemIdx(i);
		PIItem item = (PIItem)ci->m_pData;
		if (!item)
			continue;
		
		if (item->m_highlight_equipped && item->m_pInventory && item->m_pInventory->ItemFromSlot(item->BaseSlot()) == item)
			ci->m_select_equipped = true;
		else
			ci->m_select_equipped = false;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		CUIDragDropListEx* pocket		= m_pInvPocket[i];
		u32 count						= pocket->ItemsCount();
		for (u32 j = 0; j < count; j++)
		{
			CUICellItem* ci				= pocket->GetItemIdx(j);
			PIItem item					= (PIItem)ci->m_pData;
			if (!item)
				continue;
			ci->m_select_equipped		= (item->m_section_id == ACTOR_DEFS::g_quick_use_slots[i]);
		}
	}
}
