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
#include "../GrenadeLauncher.h"
#include "../trade_parameters.h"
#include "../ActorHelmet.h"
#include "../CustomOutfit.h"
#include "../CustomDetector.h"
#include "../eatable_item.h"
#include "../WeaponBinoculars.h"
#include "../WeaponKnife.h"
#include "../WeaponPistol.h"

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
#include "../item_container.h"
#include "UIInvUpgradeInfo.h"
#include "UICellCustomItems.h"

#include "scope.h"
#include "silencer.h"

void CUIActorMenu::SetActor(CInventoryOwner* io)
{
	R_ASSERT										(!IsShown());
	m_last_time										= Device.dwTimeGlobal;
	m_pActorInvOwner								= io;
	m_pActorInv										= (io) ? &io->inventory() : nullptr;
	
	if (io)
		m_ActorCharacterInfo->InitCharacter			(m_pActorInvOwner->object_id());
	else
		m_ActorCharacterInfo->ClearInfo				();
}

void CUIActorMenu::SetPartner(CInventoryOwner* io)
{
	if (io)
	{
		if (io->use_simplified_visual())
			m_PartnerCharacterInfo->ClearInfo		();
		else
			m_PartnerCharacterInfo->InitCharacter	(io->object_id());
		SetInvBox									(NULL);
		SetContainer								(NULL);
	}
	else
		m_PartnerCharacterInfo->ClearInfo			();
	
	m_pPartnerInvOwner								= io;
}

void CUIActorMenu::SetInvBox(CInventoryBox* box)
{
	if (box)
	{
		SetPartner						(NULL);
		SetContainer					(NULL);
	}

	m_pInvBox							= box;
}

void CUIActorMenu::SetContainer(MContainer* ciitem)
{
	if (ciitem)
	{
		SetPartner						(NULL);
		SetInvBox						(NULL);
	}

	m_pContainer						= ciitem;
}

void CUIActorMenu::SetBag(MContainer* bag)
{
	m_pBag								= bag;
	CUIDragDropListEx* pBagList			= (m_currMenuMode == mmTrade) ? m_pTradeActorBagList : m_pInventoryBagList;

	bool status							= !!m_pBag;
	pBagList->Show						(status);
	m_ActorWeightInfo->Show				(status);
	m_ActorWeight->Show					(status);
	m_ActorVolumeInfo->Show				(status);
	m_ActorVolume->Show					(status);
}

void CUIActorMenu::StartMenuMode(EMenuMode mode, void* partner)
{
	SetMenuMode							(mode, partner);
	ShowDialog							(true);
}

#include "GamePersistent.h"
void CUIActorMenu::SetMenuMode(EMenuMode mode, void* partner, bool forced)
{
	SetCurrentItem						(nullptr);
	m_hint_wnd->set_text				(nullptr);

	if (mode != m_currMenuMode)
	{
		switch (m_currMenuMode)
		{
		case mmUndefined:
			break;
		case mmInventory:
			DeInitInventoryMode			();
			break;
		case mmTrade:
			DeInitTradeMode				();
			break;
		case mmUpgrade:
			DeInitUpgradeMode			();
			break;
		case mmDeadBodySearch:
			DeInitDeadBodySearchMode	();
			break;
		default:
			R_ASSERT					(false);
			break;
		}
	}

	if (mode != m_currMenuMode || forced)
	{
		switch (mode)
		{
		case mmTrade:
		case mmUpgrade:
		case mmDeadBodySearch:
			SetPartner					(reinterpret_cast<CInventoryOwner*>(partner));
			break;
		case mmInvBoxSearch:
			SetInvBox					(reinterpret_cast<CInventoryBox*>(partner));
			mode						= mmDeadBodySearch;
			break;
		case mmContainerSearch:
			SetContainer				(reinterpret_cast<MContainer*>(partner));
			mode						= mmDeadBodySearch;
			break;
		default:
			SetPartner					(nullptr);
			SetInvBox					(nullptr);
			SetContainer				(nullptr);
			SetBag						(nullptr);
			break;
		}

		m_currMenuMode					= mode;
		switch (mode)
		{
		case mmUndefined:
			ResetMode					();
			break;
		case mmInventory:
			InitInventoryMode			();
			break;
		case mmTrade:
			InitTradeMode				();
			break;
		case mmUpgrade:
			InitUpgradeMode				();
			break;
		case mmDeadBodySearch:
			InitDeadBodySearchMode		();
			break;
		default:
			R_ASSERT					(false);
			break;
		}

		CurModeToScript					();
	}

	if (m_pActorInvOwner)
	{
		UpdateOutfit					();
		UpdateActor						();
	}

	if (m_currMenuMode == mmUndefined)
		GamePersistent().RestoreEffectorDOF();
	else
		GamePersistent().SetEffectorDOF({ -1.f, -0.5f, 0.f });
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
	m_upgrade_info->Draw();
}

void CUIActorMenu::Update()
{
	m_last_time = Device.dwTimeGlobal;
	switch (m_currMenuMode)
	{
	case mmUndefined:
		break;
	case mmInventory:
		CurrentGameUI()->UIMainIngameWnd->UpdateZoneMap();
		break;
	case mmTrade:
		if (m_pPartnerInvOwner->inventory().ModifyFrame() != m_trade_partner_inventory_state)
			InitPartnerInventoryContents();
		CheckDistance();
		break;
	case mmUpgrade:
		UpdateUpgradeItem();
		CheckDistance();
		break;
	case mmDeadBodySearch:
		CheckDistance();
		break;
	default:
		R_ASSERT(false);
		break;
	}
	inherited::Update();
	if (m_ItemInfo->IsEnabled())
		m_ItemInfo->Update();
	m_hint_wnd->Update();

	update_lists(true);
	update_lists(false);

	if (auto active_item = m_pActorInv->ActiveItem())
	{
		if (auto ciitem = active_item->O.getModule<MContainer>())
		{
			CHudItem* hi				= smart_cast<CHudItem*>(active_item);
			if (hi->GetState() == CHudItem::eIdle)
			{
				ToggleBag				(ciitem);
				return;
			}
		}
	}
	ToggleBag							(nullptr);
}

void CUIActorMenu::update_lists(bool clear)
{
	for (int i = 1; i <= m_slot_count; ++i)
		if (m_pInvList[i] && !m_pInvList[i]->isValid())
			(clear) ? m_pInvList[i]->ClearAll(true) : InitCellForSlot(i);

	for (int i = 0; i < m_pockets_count; ++i)
		if (!m_pInvPocket[i]->isValid())
			(clear) ? m_pInvPocket[i]->ClearAll(true) : InitPocket(i);
	
	if (!m_pDeadBodyBagList->isValid())
		(clear) ? m_pDeadBodyBagList->ClearAll(true) : init_dead_body_bag();

	if (!m_pTradeActorList->isValid())
		(clear) ? m_pTradeActorList->ClearAll(true) : init_actor_trade();

	if (!m_pTradePartnerList->isValid())
		(clear) ? m_pTradePartnerList->ClearAll(false) : update_partner_trade();

	if (!m_pTrashList->isValid())
		(clear) ? m_pTrashList->ClearAll(true) : init_vicinity();

	auto bag = GetListByType(iActorBag);
	if (!bag->isValid())
		(clear) ? bag->ClearAll(true) : init_bag();
}

void CUIActorMenu::ToggleBag(MContainer* bag)
{
	if (m_pBag != bag)
	{
		SetBag							(bag);
		if (bag && m_currMenuMode != mmUndefined)
			init_bag					();
	}
}

void CUIActorMenu::init_bag()
{
	auto bag_list						= GetListByType(iActorBag);
	bag_list->ClearAll					(true);
	TIItemContainer						items_list;
	m_pBag->AddAvailableItems			(items_list);
	items_list.sort						(InventoryUtilities::GreaterRoomInRuck);

	for (auto& item : items_list)
	{
		CUICellItem* itm				= item->getIcon();
		bag_list->SetItem				(itm);
		ColorizeItem					(itm);
	}
	
	InventoryUtilities::UpdateLabelsValues(m_ActorWeight, m_ActorVolume, nullptr, m_pBag);
	//InventoryUtilities::AlighLabels(m_ActorWeightInfo, m_ActorWeight, m_ActorVolumeInfo, m_ActorVolume);

	bag_list->setValid					(true);
}

bool CUIActorMenu::StopAnyMove()  // true = актёр не идёт при открытом меню
{
	switch (m_currMenuMode)
	{
	case mmUndefined:
	case mmInventory:
	case mmDeadBodySearch:
		return false;
	case mmTrade:
	case mmUpgrade:
		return true;
	}
	return false;
}

void CUIActorMenu::CheckDistance()
{
	CActor* actor			= smart_cast<CActor*>(m_pActorInvOwner);
	if (m_pPartnerInvOwner && !m_pPartnerInvOwner->NeedOsoznanieMode() && !m_pPartnerInvOwner->is_alive() && m_pPartnerInvOwner != actor->PersonWeLookingAt()
		|| m_pContainer && actor->ContainerWeLookingAt() != m_pContainer
		|| m_pInvBox && m_pInvBox != actor->InvBoxWeLookingAt())
	{
		g_btnHint->Discard	();
		SetMenuMode			(mmInventory);
	}
}

EDDListType CUIActorMenu::GetListType(CUIDragDropListEx* l)
{
	if (l == m_pInventoryBagList)
		return							iActorBag;

	for (auto& slot : m_pInvList)
		if (slot == l)
			return						iActorSlot;

	for (auto& pocket : m_pInvPocket)
		if (pocket == l)
			return						iActorPocket;

	if (l==m_pTradeActorBagList)		return iActorBag;
	if (l==m_pTradeActorList)			return iActorTrade;
	if (l==m_pTradePartnerBagList)		return iPartnerTradeBag;
	if (l==m_pTradePartnerList)			return iPartnerTrade;
	if (l==m_pDeadBodyBagList)			return iDeadBodyBag;
	if (l==m_pTrashList)				return iTrashSlot;

	VERIFY								(0);
	return								iInvalid;
}

CUIDragDropListEx* CUIActorMenu::GetListByType(EDDListType t)
{
	switch(t)
	{
	case iActorBag:
		if (m_currMenuMode == mmTrade)
			return						m_pTradeActorBagList;
		else
			return						m_pInventoryBagList;
		break;
	case iDeadBodyBag:
		return							m_pDeadBodyBagList;
		break;
	default:
		VERIFY2							(0, "invalid call");
	}
	return								nullptr;
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
	TryHidePropertiesBox();
	if (m_currMenuMode == mmUpgrade)
		SetupUpgradeItem();
}

void CUIActorMenu::InfoCurItem(CUICellItem* cell_item)
{
	if (!cell_item || cell_item == m_ItemInfo->getItem())
	{
		m_ItemInfo->InitItem			(nullptr);
		return;
	}

	PIItem current_item					= static_cast<PIItem>(cell_item->m_pData);
	if (GetMenuMode() == mmTrade)
	{
		CInventoryOwner* item_owner		= (current_item && current_item->m_pInventory) ? smart_cast<CInventoryOwner*>(current_item->m_pInventory->GetOwner()) : NULL;
		if (current_item && item_owner != m_pPartnerInvOwner)
		{
			shared_str					reason;
			if (CanMoveToPartner(current_item, &reason))
				m_ItemInfo->InitItem	(cell_item, m_partner_trade->GetItemPrice(cell_item, true));
			else
				m_ItemInfo->InitItem	(cell_item, u32(-1), *reason);
		}
		else
			m_ItemInfo->InitItem		(cell_item, m_partner_trade->GetItemPrice(cell_item, false));
	}
	else
		m_ItemInfo->InitItem			(cell_item, u32(-1));

	fit_in_rect							(m_ItemInfo, Frect().set( 0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT ), 10.f, 0.f);
}

// ================================================================

void CUIActorMenu::clear_highlight_lists()
{
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvSlotHighlight[i])
			m_pInvSlotHighlight[i]->Show(false);
	}

	for (int i = 0; i < m_pockets_count; i++)
		m_pInvPocket[i]->clear_select_armament();

	if (m_currMenuMode == mmTrade)
	{
		m_pTradeActorBagList->clear_select_armament();
		m_pTradeActorList->clear_select_armament();
		m_pTradePartnerBagList->clear_select_armament();
		m_pTradePartnerList->clear_select_armament();
	}
	else
		m_pInventoryBagList->clear_select_armament();

	if (m_currMenuMode == mmDeadBodySearch)
		m_pDeadBodyBagList->clear_select_armament();

	m_highlight_clear = true;
}

void CUIActorMenu::highlight_item_slot(CUICellItem* cell_item)
{
	PIItem item = (PIItem)cell_item->m_pData;
	if (!item)
		return;

	if (CUIDragDropListEx::m_drag_item)
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

	if (slot_id <= m_slot_count && m_pInvSlotHighlight[slot_id])
	{
		m_pInvSlotHighlight[slot_id]->Show(true);
		return;
	}
}

void CUIActorMenu::set_highlight_item(CUICellItem* cell_item)
{
	highlight_item_slot(cell_item);

	for (int i = 0; i < m_pockets_count; i++)
		highlight_armament(cell_item, m_pInvPocket[i]);

	if (m_currMenuMode == mmTrade)
	{
		highlight_armament(cell_item, m_pTradeActorBagList);
		highlight_armament(cell_item, m_pTradeActorList);
		highlight_armament(cell_item, m_pTradePartnerBagList);
		highlight_armament(cell_item, m_pTradePartnerList);
	}
	else
		highlight_armament(cell_item, m_pInventoryBagList);

	if (m_currMenuMode == mmDeadBodySearch)
		highlight_armament(cell_item, m_pDeadBodyBagList);

	highlight_armament(cell_item, m_pTrashList);

	m_highlight_clear = false;
}

void CUIActorMenu::highlight_armament(CUICellItem* cell_item, CUIDragDropListEx* ddlist)
{
	ddlist->clear_select_armament();

	bool ammo							= ItemCategory(cell_item->m_section, "ammo");
	bool addon							= ItemCategory(cell_item->m_section, "addon");
	bool magazine						= ItemCategory(cell_item->m_section, "magazine");
	bool addonable						= ammo || addon || magazine;
	auto ao								= smart_cast<CUIAddonOwnerCellItem*>(cell_item);
	if (!addonable && !ao)
		return;

	auto get_slot_type = [](shared_str CR$ section)
	{
		LPCSTR addon_section			= (pSettings->line_exist(section, "supplies")) ?
			pSettings->r_string(section, "supplies") :
			section.c_str();
		return							pSettings->r_string(addon_section, "slot_type");
	};

	shared_str slot_type				= (addonable) ? get_slot_type(cell_item->m_section) : 0;
	shared_str ammo_slot_type			= (magazine) ? pSettings->r_string(cell_item->m_section, "ammo_slot_type") : 0;

	for (u32 i = 0, cnt = ddlist->ItemsCount(); i < cnt; ++i)
	{
		CUICellItem* ci					= ddlist->GetItemIdx(i);
		bool ci_ammo					= ItemCategory(ci->m_section, "ammo");
		bool ci_addon					= ItemCategory(ci->m_section, "addon");
		bool ci_magazine				= ItemCategory(ci->m_section, "magazine");
		bool ci_addonable				= ci_ammo || ci_addon || ci_magazine;
		auto ci_ao						= smart_cast<CUIAddonOwnerCellItem*>(ci);
		if (!ci_addonable && !ci_ao)
			continue;

		if (ao && ci_addonable)
		{
			shared_str ci_slot_type		= (ci_addonable) ? get_slot_type(ci->m_section) : 0;
			for (auto& s : ao->Slots())
			{
				if (CAddonSlot::isCompatible(s->type, ci_slot_type))
				{
					ci->m_select_armament = true;
					break;
				}
			}
		}

		if (addonable && ci_ao)
		{
			for (auto& s : ci_ao->Slots())
			{
				if (CAddonSlot::isCompatible(s->type, slot_type))
				{
					ci->m_select_armament = true;
					break;
				}
			}
		}

		if (magazine && ci_ammo)
		{
			shared_str ci_slot_type		= get_slot_type(ci->m_section);
			if (CAddonSlot::isCompatible(ammo_slot_type, ci_slot_type))
				ci->m_select_armament	= true;
		}

		if (ammo && ci_magazine)
		{
			shared_str ci_ammo_slot_type = pSettings->r_string(ci->m_section, "ammo_slot_type");
			if (CAddonSlot::isCompatible(ci_ammo_slot_type, slot_type))
				ci->m_select_armament	= true;
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
	Actor()->resetVicinity		();
}

bool CUIActorMenu::CanSetItemToList(PIItem item, CUIDragDropListEx* l, u16& ret_slot)
{
	u16 base_slot		= item->BaseSlot();
	if (GetSlotList(base_slot) == l)
	{
		ret_slot		= base_slot;
		return			true;
	}

	if ((base_slot == PRIMARY_SLOT) && (l == m_pInvList[SECONDARY_SLOT]))
	{
		ret_slot		= SECONDARY_SLOT;
		return			true;
	}

	if ((base_slot == SECONDARY_SLOT) && (l == m_pInvList[PRIMARY_SLOT]))
	{
		ret_slot		= PRIMARY_SLOT;
		return			true;
	}

	if (l == m_pInvList[LEFT_HAND_SLOT] || l == m_pInvList[RIGHT_HAND_SLOT] || l == m_pInvList[BOTH_HANDS_SLOT])
	{
		ret_slot		= item->HandSlot();;
		return			true;
	}

	return false;
}

void CUIActorMenu::init_vicinity()
{
	auto& items_list					= Actor()->getVicinity();
	items_list.sort						(InventoryUtilities::GreaterRoomInRuck);

	for (auto& item : items_list)
	{
		CUICellItem* itm				= item->getIcon();
		m_pTrashList->SetItem			(itm);
	}

	m_pTrashList->setValid				(true);
}
