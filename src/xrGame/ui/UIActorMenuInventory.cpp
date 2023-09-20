#include "stdafx.h"
#include "UIActorMenu.h"
#include "../inventory.h"
#include "../inventoryOwner.h"
#include "UIInventoryUtilities.h"
#include "UIItemInfo.h"
#include "../Level.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICellCustomItems.h"
#include "UIItemInfo.h"
#include "UIFrameLineWnd.h"
#include "UIPropertiesBox.h"
#include "UIListBoxItem.h"
#include "UIMainIngameWnd.h"
#include "UIGameCustom.h"
#include "eatable_item_object.h"

#include "../silencer.h"
#include "../scope.h"
#include "../grenadelauncher.h"
#include "../Artefact.h"
#include "../eatable_item.h"
#include "../BottleItem.h"
#include "../WeaponMagazined.h"
#include "../Medkit.h"
#include "../Antirad.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../UICursor.h"
#include "../MPPlayersBag.h"
#include "../player_hud.h"
#include "../CustomDetector.h"
#include "../PDA.h"
#include "../ActorBackpack.h"
#include "../actor_defs.h"

using namespace luabind; //Alundaio

void move_item_from_to(u16 from_id, u16 to_id, u16 what_id);

void CUIActorMenu::InitInventoryMode()
{
	m_pInventoryBagList->Show(false);
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvList[i])
			m_pInvList[i]->Show(true);
	}

	for (u16 i = 0; i < m_pockets_count; i++)
	{
		m_pInvPocket[i]->Show		(true);
		m_pPocketOver[i]->Show		(!m_pActorInvOwner->inventory().PocketPresent(i));
	}

	m_pTrashList->Show					(true);

	InitInventoryContents				(m_pInventoryBagList);

	VERIFY( CurrentGameUI() );
	CurrentGameUI()->UIMainIngameWnd->ShowZoneMap(true);
}

void CUIActorMenu::UpdatePocketsPresence()
{
	for (u16 i = 0; i < m_pockets_count; i++)
		m_pPocketOver[i]->Show(!m_pActorInvOwner->inventory().PocketPresent(i));
}

void CUIActorMenu::DeInitInventoryMode()
{
}

void CUIActorMenu::ActivateItem(CUICellItem* itm, u16 return_place, u16 return_slot)
{
	PIItem item										= (PIItem)itm->m_pData;
	m_pActorInvOwner->inventory().ActivateItem		(item, return_place, return_slot);
	if (item->parent_id() != m_pActorInvOwner->object_id())
		ToBag										(itm, false);
}

void CUIActorMenu::SendEvent_PickUpItem(PIItem pItem, u16 type, u16 slot_id)
{
	pItem->m_ItemCurrPlaceBackup			= pItem->m_ItemCurrPlace;
	pItem->m_ItemCurrPlaceBackup.type		= type;
	pItem->m_ItemCurrPlaceBackup.slot_id	= slot_id;
	if (pItem->parent_id() == u16(-1))
		Game().SendPickUpEvent				(m_pActorInvOwner->object_id(), pItem->object_id());
	else
		move_item_from_to					(pItem->parent_id(), m_pActorInvOwner->object_id(), pItem->object_id());
}

void CUIActorMenu::SendEvent_Item_Eat(PIItem pItem, u16 recipient)
{
	if(pItem->parent_id()!=recipient)
		move_item_from_to			(pItem->parent_id(), recipient, pItem->object_id());

	NET_Packet						P;
	CGameObject::u_EventGen			(P, GEG_PLAYER_ITEM_EAT, recipient);
	P.w_u16							(pItem->object().ID());
	CGameObject::u_EventSend		(P);
}

void CUIActorMenu::SendEvent_Item_Drop(PIItem pItem, u16 recipient)
{
	R_ASSERT(pItem->parent_id() == recipient);

	NET_Packet						P;
	pItem->object().u_EventGen		(P,GE_OWNERSHIP_REJECT,pItem->parent_id());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
}

void CUIActorMenu::DropAllCurrentItem()
{
	if ( CurrentIItem() && !CurrentIItem()->IsQuestItem() )
	{
		u32 const cnt = CurrentItem()->ChildsCount();
		for( u32 i = 0; i < cnt; ++i )
		{
			CUICellItem*	itm  = CurrentItem()->PopChild(NULL);
			PIItem			iitm = (PIItem)itm->m_pData;
			SendEvent_Item_Drop( iitm, m_pActorInvOwner->object_id() );
		}

		SendEvent_Item_Drop( CurrentIItem(), m_pActorInvOwner->object_id() );
	}
	SetCurrentItem								(NULL);
}

bool CUIActorMenu::DropAllItemsFromRuck( bool quest_force )
{
	if ( !IsShown() || !m_pInventoryBagList || m_currMenuMode != mmInventory )
	{
		return false;
	}

	u32 const ci_count = m_pInventoryBagList->ItemsCount();
	for ( u32 i = 0; i < ci_count; ++i )
	{
		CUICellItem* ci = m_pInventoryBagList->GetItemIdx( i );
		VERIFY( ci );
		PIItem item = (PIItem)ci->m_pData;
		VERIFY( item );

		if ( !quest_force && item->IsQuestItem() )
		{
			continue;
		}

		u32 const cnt = ci->ChildsCount();
		for( u32 j = 0; j < cnt; ++j )
		{
			CUICellItem*	child_ci   = ci->PopChild(NULL);
			PIItem			child_item = (PIItem)child_ci->m_pData;
			SendEvent_Item_Drop( child_item, m_pActorInvOwner->object_id() );
		}
		SendEvent_Item_Drop( item, m_pActorInvOwner->object_id() );
	}

	SetCurrentItem( NULL );
	return true;
}

bool FindItemInList(CUIDragDropListEx* lst, PIItem pItem, CUICellItem*& ci_res)
{
	u32 count = lst->ItemsCount();
	for (u32 i=0; i<count; ++i)
	{
		CUICellItem* ci				= lst->GetItemIdx(i);
		for(u32 j=0; j<ci->ChildsCount(); ++j)
		{
			CUIInventoryCellItem* ici = smart_cast<CUIInventoryCellItem*>(ci->Child(j));
			if(ici->object()==pItem)
			{
				ci_res = ici;
				//lst->RemoveItem(ci,false);
				return true;
			}
		}

		CUIInventoryCellItem* ici = smart_cast<CUIInventoryCellItem*>(ci);
		if(ici->object()==pItem)
		{
			ci_res = ci;
			//lst->RemoveItem(ci,false);
			return true;
		}
	}
	return false;
}

bool RemoveItemFromList(CUIDragDropListEx* lst, PIItem pItem)
{// fixme
	CUICellItem*	ci	= NULL;
	if(FindItemInList(lst, pItem, ci))
	{
		R_ASSERT		(ci);

		CUICellItem* dying_cell = lst->RemoveItem	(ci, false);
		xr_delete(dying_cell);

		return			true;
	}else
		return			false;
}

void CUIActorMenu::OnInventoryAction(PIItem pItem, u16 action_type)
{
	CUIDragDropListEx* all_lists[] =
	{
		m_pInventoryBagList,
		m_pTradeActorBagList,
		m_pTradeActorList,
		m_pDeadBodyBagList,
		m_pTrashList,
		NULL
	};

	switch (action_type)
	{
		case GE_TRADE_BUY :
		case GE_OWNERSHIP_TAKE :
			{
				u32 i			= 0;
				bool b_already	= false;

				CUIDragDropListEx* lst_to_add		= NULL;
				SInvItemPlace pl						= pItem->m_ItemCurrPlace;
#ifndef MASTER_GOLD
				Msg("item place [%d]", pl);
#endif // #ifndef MASTER_GOLD

				if (pl.type==eItemPlaceSlot)
					lst_to_add						= GetSlotList(pl.slot_id);
				else if (pl.type == eItemPlacePocket)
					lst_to_add						= m_pInvPocket[pl.slot_id];
				else/* if(pl.type==eItemPlaceRuck)*/
				{
					if (pItem->parent_id() == m_pActorInvOwner->object_id())
						lst_to_add						= GetListByType(iActorBag);
					else
						lst_to_add						= GetListByType(iDeadBodyBag);
				}


				while ( all_lists[i] )
				{
					CUIDragDropListEx*	curr = all_lists[i];
					CUICellItem*		ci   = NULL;
					if ( FindItemInList(curr, pItem, ci) )
					{
						if ( lst_to_add != curr )
							RemoveItemFromList(curr, pItem);
						else
							b_already = true;
						//break;
					}
					++i;
				}

				for (u8 i = 1; i <= m_slot_count; ++i)
				{
					CUIDragDropListEx*	curr = m_pInvList[i];
					if (curr)
					{
						CUICellItem* ci = NULL;
						if (FindItemInList(curr, pItem, ci))
						{
							if (lst_to_add != curr)
								RemoveItemFromList(curr, pItem);
							else
								b_already = true;
						}
					}
				}

				for (u8 i = 0; i < m_pockets_count; i++)
				{
					CUIDragDropListEx*				pocket = m_pInvPocket[i];
					CUICellItem* ci					= NULL;
					if (FindItemInList(pocket, pItem, ci))
					{
						if (lst_to_add != pocket)
							RemoveItemFromList		(pocket, pItem);
						else
							b_already				= true;
					}
				}

				CUICellItem*		ci   = NULL;
				if(GetMenuMode()==mmDeadBodySearch && FindItemInList(m_pDeadBodyBagList, pItem, ci))
					break;

				if ( !b_already )
				{
					if ( lst_to_add )
					{
						CUICellItem* itm	= create_cell_item(pItem);
						lst_to_add->SetItem(itm);
						if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
							ColorizeItem(itm, !CanMoveToPartner(pItem));
					}
				}
			}break;
		case GE_TRADE_SELL :
		case GE_OWNERSHIP_REJECT :
			{
				if(CUIDragDropListEx::m_drag_item)
				{
					CUIInventoryCellItem* ici = smart_cast<CUIInventoryCellItem*>(CUIDragDropListEx::m_drag_item->ParentItem());
					R_ASSERT(ici);
					if(ici->object()==pItem)
					{
						CUIDragDropListEx*	_drag_owner		= ici->OwnerList();
						_drag_owner->DestroyDragItem		();
					}
				}

				u32 i = 0;
				while(all_lists[i])
				{
					CUIDragDropListEx* curr = all_lists[i];
					if(RemoveItemFromList(curr, pItem))
					{
						break;
					}
					++i;
				}

				for (u8 i = 1; i <= m_slot_count; ++i)
				{
					CUIDragDropListEx*	curr = m_pInvList[i];
					if (curr)
					{
						if (RemoveItemFromList(curr, pItem))
							break;
					}
				}

				for (u8 i = 0; i < m_pockets_count; i++)
				{
					CUIDragDropListEx*		pocket = m_pInvPocket[i];
					if (pocket && RemoveItemFromList(pocket, pItem))
						break;
				}
			}break;
	}

	UpdateItemsPlace();
	UpdateConditionProgressBars();
}

void CUIActorMenu::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd										(eAttachAddon);
	R_ASSERT									(item_to_upgrade);
	if (OnClient())
	{
		NET_Packet								P;
		CGameObject::u_EventGen					(P, GE_ADDON_ATTACH, item_to_upgrade->object_id());
		P.w_u16									(CurrentIItem()->object_id());
		CGameObject::u_EventSend				(P);
	}

	item_to_upgrade->Attach						(CurrentIItem(), true);
	m_pActorInvOwner->inventory().Ruck			(item_to_upgrade);

	SetCurrentItem								(NULL);
}

void CUIActorMenu::DetachAddon(LPCSTR addon_name)
{
	PlaySnd										(eDetachAddon);
	if (OnClient())
	{
		NET_Packet								P;
		CGameObject::u_EventGen					(P, GE_ADDON_DETACH, CurrentIItem()->object_id());
		P.w_stringZ								(addon_name);
		CGameObject::u_EventSend				(P);
	}
	
	CurrentIItem()->Detach						(addon_name, true);
	m_pActorInvOwner->inventory().Ruck			(CurrentIItem());
}

void CUIActorMenu::InitCellForSlot( u16 slot_idx )
{
	PIItem item	= m_pActorInvOwner->inventory().ItemFromSlot(slot_idx);
	if ( !item )
		return;

	CUIDragDropListEx* curr_list = GetSlotList( slot_idx );
	if (!curr_list)
		return;

	CUICellItem* cell_item			= create_cell_item( item );
	curr_list->SetItem( cell_item );
	if ( m_currMenuMode == mmTrade && m_pPartnerInvOwner )
		ColorizeItem( cell_item, !CanMoveToPartner( item ) );
}

void CUIActorMenu::InitPocket(u16 pocket_idx)
{
	TIItemContainer		pocket_list;
	pocket_list			= m_pActorInvOwner->inventory().m_pockets[pocket_idx];
	std::sort(pocket_list.begin(), pocket_list.end(), InventoryUtilities::GreaterRoomInRuck);

	LPCSTR sect										= ACTOR_DEFS::g_quick_use_slots[pocket_idx];
	for (TIItemContainer::iterator itb = pocket_list.begin(), ite = pocket_list.end(); itb != ite; ++itb)
	{
		if ((*itb)->m_section_id == sect)
		{
			CUICellItem* itm						= create_cell_item(*itb);
			m_pInvPocket[pocket_idx]->SetItem		(itm);
			if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
				ColorizeItem						(itm, !CanMoveToPartner(*itb));
		}
	}


	for (TIItemContainer::iterator itb = pocket_list.begin(), ite = pocket_list.end(); itb != ite; ++itb)
	{
		if (smart_cast<CMPPlayersBag*>(&(*itb)->object()) || (*itb)->m_section_id == sect)
			continue;

		CUICellItem* itm						= create_cell_item(*itb);
		m_pInvPocket[pocket_idx]->SetItem		(itm);
		if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
			ColorizeItem						(itm, !CanMoveToPartner(*itb));
	}
}

void CUIActorMenu::InitInventoryContents(CUIDragDropListEx* pBagList)
{
	ClearAllLists				();
	m_pMouseCapturer			= NULL;
	m_UIPropertiesBox->Hide		();
	SetCurrentItem				(NULL);

	//Slots
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvList[i])
			InitCellForSlot(i);
		else if (!m_pActorInvOwner->inventory().SlotIsPersistent(i))
			InitCellForSlot(i);
	}
	
	for (u8 i = 0; i < m_pockets_count; i++)
		InitPocket(i);

	if (!m_pActorInvOwner->inventory().m_bRuckAllowed)
	{
		PIItem container			= m_pActorInvOwner->inventory().ItemFromSlot(BOTH_HANDS_SLOT);
		if (container && smart_cast<CBackpack*>(container))
			ToggleRuckContainer		(container);
	}
	
	TIItemContainer ruck_list	= m_pActorInvOwner->inventory().m_ruck;
	std::sort					(ruck_list.begin(), ruck_list.end(), InventoryUtilities::GreaterRoomInRuck );
	for (TIItemContainer::iterator itb = ruck_list.begin(), ite = ruck_list.end(); itb != ite; ++itb)
	{
		CMPPlayersBag* bag		= smart_cast<CMPPlayersBag*>( &(*itb)->object() );
		if (bag)
			continue;

		CUICellItem* itm		= create_cell_item(*itb);
		pBagList->SetItem		(itm);
		if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
			ColorizeItem		(itm, !CanMoveToPartner(*itb));
	}
}

void CUIActorMenu::ToggleRuckContainer(PIItem container)
{
	((m_currMenuMode == mmTrade) ? m_pTradeActorBagList : m_pInventoryBagList)->Show(!!container);
	if (m_currMenuMode != mmUndefined)
		UpdateActor();
	if (!container || m_currMenuMode == mmUndefined)
		return;

	luabind::functor<u16>											funct;
	ai().script_engine().functor									("xmod_container_manager.get_cont_vbox", funct);
	u16 vbox_id														= funct(container->object_id());
	CInventoryBox* vbox												= smart_cast<CInventoryBox*>(Level().Objects.net_Find(vbox_id));
	if (vbox)
	{
		TIItemContainer& ruck_vbox = m_pActorInvOwner->inventory().m_ruck_vbox;
		for (xr_vector<u16>::const_iterator I = vbox->m_items.begin(), E = vbox->m_items.end(); I != E; ++I)
		{
			PIItem item												= smart_cast<PIItem>(Level().Objects.net_Find(*I));
			ruck_vbox.push_back										(item);
		}
		std::sort													(ruck_vbox.begin(), ruck_vbox.end(), InventoryUtilities::GreaterRoomInRuck);
		for (TIItemContainer::iterator it = ruck_vbox.begin(), ite = ruck_vbox.end(); it != ite; ++it)
			SendEvent_PickUpItem									(*it, eItemPlaceRuck);
	}
	m_pActorInvOwner->inventory().m_iRuckVboxID						= vbox_id;
	m_pActorInvOwner->inventory().m_pContainer						= container;
}

bool CUIActorMenu::ToSlotScript(CScriptGameObject* GO, bool force_place, u16 slot_id)
{
	CInventoryItem* iitem = smart_cast<CInventoryItem*>(GO->object().dcast_CObject());

	if (!iitem || !m_pActorInvOwner->inventory().InRuck(iitem))
		return false;

	CUIDragDropListEx* invlist = GetListByType(iActorBag);
	CUICellContainer* c = invlist->GetContainer();
	CUIWindow::WINDOW_LIST child_list = c->GetChildWndList();

	for (WINDOW_LIST_it it = child_list.begin(); child_list.end() != it; ++it)
	{
		CUICellItem* i = (CUICellItem*)(*it);
		PIItem	pitm = (PIItem)i->m_pData;
		if (pitm == iitem)
		{
			ToSlot(i, force_place, slot_id);
			return true;
		}
	}
	return false;
}

bool CUIActorMenu::ToSlot(u16 slot_id, PIItem item)
{
	if (item->CurrPlace() == eItemPlaceSlot)
	{
		CUIDragDropListEx* list		= GetSlotList(item->CurrSlot());
		CUICellItem* itm			= list->GetItemIdx(0);
		return						ToSlot(itm, false, slot_id);
	}
	else if (item->CurrPlace() == eItemPlaceRuck)
	{
		CUICellItem*				ci;
		if (FindItemInList(GetListByType(iActorBag), item, ci))
			return					ToSlot(ci, false, slot_id);
	}
	else if (item->CurrPlace() == eItemPlacePocket)
	{
		CUICellItem*				ci;
		if (FindItemInList(m_pInvPocket[item->CurrPocket()], item, ci))
			return					ToSlot(ci, false, slot_id);
	}

	return m_pActorInvOwner->inventory().Slot(slot_id, item);
}

bool CUIActorMenu::ToSlot(CUICellItem* itm, bool force_place, u16 slot_id)
{
	CUIDragDropListEx* old_owner		= itm->OwnerList();
	PIItem iitem						= (PIItem)itm->m_pData;
	bool b_own_item						= (iitem->parent_id() == m_pActorInvOwner->object_id());

	if (slot_id == HELMET_SLOT)
	{
		CCustomOutfit* pOutfit		= m_pActorInvOwner->GetOutfit();
		if (pOutfit && !pOutfit->bIsHelmetAvaliable)
			return					false;
	}
	
	if (iitem == m_pActorInvOwner->inventory().ActiveItem() && m_pActorInvOwner->inventory().m_iNextActiveItemID == iitem->object_id() ||
		iitem == m_pActorInvOwner->inventory().LeftItem() && m_pActorInvOwner->inventory().m_iNextLeftItemID == iitem->object_id())
	{
		m_pActorInvOwner->inventory().ActivateItem		(iitem, eItemPlaceSlot, slot_id);
		return											true;
	}

	if (m_pActorInvOwner->inventory().CanPutInSlot(iitem, slot_id))
	{
		CUIDragDropListEx* new_owner		= GetSlotList(slot_id);
		if (!new_owner)
			return							true;
		
		if (slot_id == OUTFIT_SLOT)
		{
			CCustomOutfit* pOutfit			= smart_cast<CCustomOutfit*>(iitem);
			if (pOutfit && !pOutfit->bIsHelmetAvaliable)
			{
				CUIDragDropListEx* helmet_list	= GetSlotList(HELMET_SLOT);
				if (helmet_list && helmet_list->ItemsCount() == 1)
				{
					CUICellItem* helmet_cell	= helmet_list->GetItemIdx(0);
					ToBag						(helmet_cell, false);
				}
			}
		}

		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner == new_owner));
		while (i->ChildsCount())
		{
			CUICellItem* child				= i->PopChild(NULL);
			old_owner->SetItem				(child);
		}
		if (!new_owner->CanSetItem(i))
			return							ToSlot(i, true, slot_id);
		new_owner->SetItem					(i);
		
		if (b_own_item)
			m_pActorInvOwner->inventory().Slot		(slot_id, iitem);
		else
			SendEvent_PickUpItem					(iitem, eItemPlaceSlot, slot_id);

		return true;
	}
	else	// in case slot is busy
	{
		if ( !force_place || slot_id == NO_ACTIVE_SLOT ) 
			return false;

		if ( m_pActorInvOwner->inventory().SlotIsPersistent(slot_id) && slot_id != DETECTOR_SLOT  )
			return false;

		if (slot_id == PRIMARY_SLOT)
		{
			if (m_pActorInvOwner->inventory().CanPutInSlot(iitem, SECONDARY_SLOT))
				return ToSlot(itm, force_place, SECONDARY_SLOT);
		}
		else if (slot_id == SECONDARY_SLOT)
		{
			if (m_pActorInvOwner->inventory().CanPutInSlot(iitem, PRIMARY_SLOT))
				return ToSlot(itm, force_place, PRIMARY_SLOT);
		}
		
		CUIDragDropListEx* slot_list;
		if (CUIDragDropListEx::m_drag_item && CUIDragDropListEx::m_drag_item->BackList())
			slot_list = CUIDragDropListEx::m_drag_item->BackList();
		else 
			slot_list = GetSlotList(slot_id);
		
		if (!slot_list)
			return false;

		PIItem	_iitem = m_pActorInvOwner->inventory().ItemFromSlot(slot_id);

		if (slot_list != GetListByType(iActorBag))
		{
			if (!slot_list->ItemsCount() == 1)
				return false;

			CUICellItem* slot_cell = slot_list->GetItemIdx(0);
			if (!slot_cell)
				return false;
			
			if ((PIItem)slot_cell->m_pData != _iitem)
				return false;

			if (!ToSlot(slot_cell, false, _iitem->BaseSlot()))
			{
				if (!ToBag(slot_cell, false))
					return false;
			}
		}
		else
		{
			//Alundaio: Since the player's inventory is being used as a slot we need to search for cell with matching m_pData
			CUICellContainer* c = slot_list->GetContainer();
			CUIWindow::WINDOW_LIST child_list = c->GetChildWndList();

			for (WINDOW_LIST_it it = child_list.begin(); child_list.end() != it; ++it)
			{
				CUICellItem* i = (CUICellItem*)(*it);
				PIItem	pitm = (PIItem)i->m_pData;
				if (pitm == _iitem)
				{
					if (ToBag(i, false))
					{
						break;
					}
					else
						return false;
				}
			}

			return ToSlot(itm, false, slot_id);
		}

		if(b_own_item && slot_id == DETECTOR_SLOT)
		{
			if (ToSlot(itm, false, slot_id))
			{
				CCustomDetector* det = smart_cast<CCustomDetector*>(iitem);
				if (det)
					det->ToggleDetector(g_player_hud->attached_item(0)!=NULL);
				return true;
			}
			return false;
		}

		return ToSlot(itm, false, slot_id);
	}
}

bool CUIActorMenu::ToBag(PIItem item)
{
	if (item->CurrPlace() == eItemPlaceSlot)
	{
		CUIDragDropListEx* list		= GetSlotList(item->CurrSlot());
		CUICellItem* itm			= list->GetItemIdx(0);
		return						ToBag(itm, false);
	}
	else if (item->CurrPlace() == eItemPlacePocket)
	{
		CUICellItem*				ci;
		if (FindItemInList(m_pInvPocket[item->CurrPocket()], item, ci))
			return					ToBag(ci, false);
	}

	return m_pActorInvOwner->inventory().Ruck(item);
}

bool CUIActorMenu::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem iitem = (PIItem)itm->m_pData;

	if (!m_pActorInvOwner->inventory().CanPutInRuck(iitem))
		return											false;
	if ((iitem == m_pActorInvOwner->inventory().ActiveItem() && m_pActorInvOwner->inventory().m_iNextActiveItemID == iitem->object_id()) ||
		(iitem == m_pActorInvOwner->inventory().LeftItem() && m_pActorInvOwner->inventory().m_iNextLeftItemID == iitem->object_id()))
	{
		m_pActorInvOwner->inventory().ActivateItem		(iitem, eItemPlaceRuck);
		return											true;
	}

	CUIDragDropListEx* old_owner		= itm->OwnerList();
	CUIDragDropListEx* new_owner		= NULL;
	if (b_use_cursor_pos)
	{
		new_owner						= CUIDragDropListEx::m_drag_item->BackList();
		VERIFY							(GetListType(new_owner) == iActorBag);
	}
	else
		new_owner						= GetListByType(iActorBag);
	bool b_already						= (old_owner == new_owner);
	CUICellItem* i						= old_owner->RemoveItem(itm, b_already);
	if (b_use_cursor_pos)
		new_owner->SetItem				(i, old_owner->GetDragItemPosition());
	else
		new_owner->SetItem				(i);
	if (b_already)
		return							true;

	if (iitem->parent_id() == m_pActorInvOwner->object_id())
		m_pActorInvOwner->inventory().Ruck		(iitem);
	else
		SendEvent_PickUpItem					(iitem, eItemPlaceRuck);

	if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
		ColorizeItem(itm, !CanMoveToPartner(iitem));

	return true;
}

bool CUIActorMenu::ToPocket(PIItem item, u16 pocket_id)
{
	if (item->CurrPlace() == eItemPlaceSlot)
	{
		CUIDragDropListEx* list		= GetSlotList(item->CurrSlot());
		CUICellItem* itm			= list->GetItemIdx(0);
		return						ToPocket(itm, false, pocket_id);
	}
	else if (item->CurrPlace() == eItemPlaceRuck)
	{
		CUICellItem*				ci;
		if (FindItemInList(GetListByType(iActorBag), item, ci))
			return					ToPocket(ci, false, pocket_id);
	}
	else if (item->CurrPlace() == eItemPlacePocket)
	{
		CUICellItem*				ci;
		if (FindItemInList(m_pInvPocket[item->CurrPocket()], item, ci))
			return					ToPocket(ci, false, pocket_id);
	}

	return m_pActorInvOwner->inventory().Pocket(item, pocket_id);
}

bool CUIActorMenu::ToPocket(CUICellItem* itm, bool b_use_cursor_pos, u16 pocket_id)
{
	PIItem iitem = (PIItem)itm->m_pData;
	
	if (!m_pActorInvOwner->inventory().CanPutInPocket(iitem, pocket_id))
		return											false;
	if ((iitem == m_pActorInvOwner->inventory().ActiveItem() && m_pActorInvOwner->inventory().m_iNextActiveItemID == iitem->object_id()) ||
		(iitem == m_pActorInvOwner->inventory().LeftItem() && m_pActorInvOwner->inventory().m_iNextLeftItemID == iitem->object_id()))
	{
		m_pActorInvOwner->inventory().ActivateItem		(iitem, eItemPlacePocket, pocket_id);
		return											true;
	}

	CUIDragDropListEx* old_owner		= itm->OwnerList();
	CUIDragDropListEx* new_owner		= m_pInvPocket[pocket_id];
	bool b_already						= (old_owner==new_owner);
	CUICellItem* i						= old_owner->RemoveItem(itm, b_already);
	if (b_use_cursor_pos)
		new_owner->SetItem				(i, old_owner->GetDragItemPosition());
	else
		new_owner->SetItem				(i);
	if (b_already)
		return							true;
	
	if (iitem->parent_id() == m_pActorInvOwner->object_id())
		m_pActorInvOwner->inventory().Pocket		(iitem, pocket_id);
	else
		SendEvent_PickUpItem						(iitem, eItemPlacePocket, pocket_id);

	return true;
}

CUIDragDropListEx* CUIActorMenu::GetSlotList(u16 slot_idx)
{
	if ( slot_idx == NO_ACTIVE_SLOT )
		return NULL;

	if (m_pInvList[slot_idx])
		return m_pInvList[slot_idx];

	if ( m_currMenuMode == mmTrade )
		return m_pTradeActorBagList;
	return m_pInventoryBagList;
}

bool CUIActorMenu::TryUseItem(CUICellItem* cell_itm)
{
	if (!cell_itm)
		return false;
	PIItem item	= (PIItem)cell_itm->m_pData;
	
	CBottleItem*	pBottleItem		= smart_cast<CBottleItem*>	(item);
	CMedkit*		pMedkit			= smart_cast<CMedkit*>		(item);
	CAntirad*		pAntirad		= smart_cast<CAntirad*>		(item);
	CEatableItem*	pEatableItem	= smart_cast<CEatableItem*>	(item);

	if (!(pMedkit || pAntirad || pEatableItem || pBottleItem))
		return false;
	if (!item->Useful())
		return false;

	u16 recipient = m_pActorInvOwner->object_id();
	if (item->parent_id() != recipient)
		cell_itm->OwnerList()->RemoveItem(cell_itm, false);

	SendEvent_Item_Eat		(item, recipient);
	PlaySnd					(eItemUse);

	return true;
}

bool CUIActorMenu::OnItemDropped(PIItem itm, CUIDragDropListEx* new_owner, CUIDragDropListEx* old_owner)
{
	CUICellItem* _citem	= (new_owner->ItemsCount() == 1) ? new_owner->GetItemIdx(0) : NULL;
	PIItem _iitem		= _citem ? (PIItem)_citem->m_pData : NULL;

	if (!_iitem || !_iitem->InHands() || !_iitem->CanAttach(itm) || old_owner != m_pInventoryBagList)
		return			false;

	AttachAddon			(_iitem);
	return				true;
}

void CUIActorMenu::TryHidePropertiesBox()
{
	if (m_UIPropertiesBox->IsShown())
		m_UIPropertiesBox->Hide();
}

u16 CUIActorMenu::GetPocketIdx(CUIDragDropListEx* pocket)
{
	for (u16 i = 0; i < m_pockets_count; i++)
	{
		if (m_pInvPocket[i] == pocket)
			return i;
	}

	return 0;
}

u16 ho_equipped, ho;
void CUIActorMenu::ActivatePropertiesBox()
{
	TryHidePropertiesBox();
	if (!(m_currMenuMode == mmInventory || m_currMenuMode == mmDeadBodySearch || m_currMenuMode == mmUpgrade || m_currMenuMode == mmTrade))
		return;

	PIItem item = CurrentIItem();
	if (!item)
		return;

	CUICellItem* cell_item = CurrentItem();
	m_UIPropertiesBox->RemoveAll();
	bool b_show = false;
	
	ho									= (smart_cast<CCustomOutfit*>(item) || smart_cast<CHelmet*>(item));
	ho_equipped							= (ho && item->CurrSlot() == item->BaseSlot());
	if (m_currMenuMode != mmTrade && m_pActorInvOwner->inventory().CanPutInSlot(item, item->HandSlot()) && !ho_equipped)
	{
		m_UIPropertiesBox->AddItem		("st_to_hands", NULL, INVENTORY_TO_HANDS_ACTION);
		b_show							= true;
	}

	CUIDragDropListEx* list					= cell_item->OwnerList();
	if (GetListType(list) == iActorPocket)
	{
		u16 idx								= GetPocketIdx(list);
		if (item->m_section_id == ACTOR_DEFS::g_quick_use_slots[idx])
		{
			m_UIPropertiesBox->AddItem		("st_from_quick", NULL, INVENTORY_FROM_QUICK_ACTION);
			b_show = true;
		}
		else
		{
			m_UIPropertiesBox->AddItem		("st_to_quick", NULL, INVENTORY_TO_QUICK_ACTION);
			b_show = true;
		}
	}

	if (m_currMenuMode == mmInventory || m_currMenuMode == mmDeadBodySearch)
	{
		PropertiesBoxForSlots(item, b_show);
		PropertiesBoxForWeapon(cell_item, item, b_show);
		PropertiesBoxForAddon(item, b_show);
		PropertiesBoxForUsing(item, b_show);
		PropertiesBoxForPlaying(item, b_show);
		if (m_currMenuMode == mmInventory)
			PropertiesBoxForDrop( cell_item, item, b_show );
	}
	else if ( m_currMenuMode == mmUpgrade )
		PropertiesBoxForRepair(item, b_show);

	//Alundaio: Ability to donate item to npc during trade
	else if (m_currMenuMode == mmTrade)
	{
		CUIDragDropListEx* invlist = GetListByType(iActorBag);
		if (invlist->IsOwner(cell_item))
			PropertiesBoxForDonate(item, b_show);
	}
	//-Alundaio

	if (b_show)
	{
		m_UIPropertiesBox->AutoUpdateSize();

		Fvector2 cursor_pos;
		Frect						vis_rect;
		GetAbsoluteRect				(vis_rect);
		cursor_pos					= GetUICursor().GetCursorPosition();
		cursor_pos.sub				(vis_rect.lt);
		m_UIPropertiesBox->Show		(vis_rect, cursor_pos);
		PlaySnd						(eProperties);
	}
}

void CUIActorMenu::PropertiesBoxForSlots(PIItem item, bool& b_show)
{
	CInventory& inventory			= m_pActorInvOwner->inventory();
	u16 base_slot					= item->BaseSlot();

	if (!ho && base_slot != NO_ACTIVE_SLOT && inventory.CanPutInSlot(item, base_slot))
	{
		m_UIPropertiesBox->AddItem		("st_move_to_slot", NULL, INVENTORY_TO_SLOT_ACTION);
		b_show							= true;
	}

	if (item->Ruck() && inventory.CanPutInRuck(item) && (item->CurrSlot() != NO_ACTIVE_SLOT || m_currMenuMode == mmDeadBodySearch) && !ho_equipped)
	{
		m_UIPropertiesBox->AddItem		("st_move_to_ruck", NULL, INVENTORY_TO_BAG_ACTION);
		b_show							= true;
	}
}

void CUIActorMenu::PropertiesBoxForWeapon(CUICellItem* cell_item, PIItem item, bool& b_show)
{
	if (!item->InHands())
		return;

	//отсоединение аддонов от вещи
	CWeapon* pWeapon = smart_cast<CWeapon*>(item);
	if (!pWeapon)
		return;

	if (pWeapon->GrenadeLauncherAttachable())
	{
		if (pWeapon->IsGrenadeLauncherAttached())
		{
			m_UIPropertiesBox->AddItem("st_detach_gl", NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			b_show			= true;
		}
	}

	if (pWeapon->ScopeAttachable())
	{
		if (pWeapon->IsScopeAttached())
		{
			m_UIPropertiesBox->AddItem("st_detach_scope", NULL, INVENTORY_DETACH_SCOPE_ADDON);
			b_show			= true;
		}
	}

	if (pWeapon->SilencerAttachable())
	{
		if (pWeapon->IsSilencerAttached())
		{
			m_UIPropertiesBox->AddItem("st_detach_silencer", NULL, INVENTORY_DETACH_SILENCER_ADDON);
			b_show			= true;
		}
	}

	if (smart_cast<CWeaponMagazined*>(pWeapon))
	{
		if (pWeapon->MagazineIndex())
		{
			m_UIPropertiesBox->AddItem("st_unload_magazine", NULL, INVENTORY_UNLOAD_MAGAZINE);
			b_show = true;
		}
		else if (pWeapon->GetAmmoElapsed())
		{
			m_UIPropertiesBox->AddItem("st_discharge_weapon", NULL, INVENTORY_DISCHARGE_WEAPON);
			b_show = true;
		}
	}
}

#include "../string_table.h"
void CUIActorMenu::PropertiesBoxForAddon(PIItem item, bool& b_show)
{
	PIItem				active_item			= m_pActorInvOwner->inventory().ActiveItem();
	if (!active_item)
		return;
	CScope*				pScope				= smart_cast<CScope*>			(item);
	CSilencer*			pSilencer			= smart_cast<CSilencer*>		(item);
	CGrenadeLauncher*	pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>	(item);

	if (pScope)
	{
		if (active_item->CanAttach(pScope))
		{
			shared_str str = CStringTable().translate("st_attach_scope_to");
			str.printf("%s %s", str.c_str(), active_item->m_name.c_str());
			m_UIPropertiesBox->AddItem(str.c_str(), (void*)active_item, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		}
		return;
	}

	if (pSilencer)
	{
		if (active_item->CanAttach(pSilencer))
		{
			shared_str str = CStringTable().translate("st_attach_silencer_to");
			str.printf("%s %s", str.c_str(), active_item->m_name.c_str());
			m_UIPropertiesBox->AddItem(str.c_str(), (void*)active_item, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		}
		return;
	}

	if (pGrenadeLauncher)
	{
		if (active_item->CanAttach(pGrenadeLauncher))
		{
			shared_str str = CStringTable().translate("st_attach_gl_to");
			str.printf("%s %s", str.c_str(), active_item->m_name.c_str());
			m_UIPropertiesBox->AddItem(str.c_str(), (void*)active_item, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		}
	}
}

void CUIActorMenu::PropertiesBoxForUsing(PIItem item, bool& b_show)
{
	if (item->InHands() && item->cast_hud_item()->GetState() != CHUDState::eIdle)
		return;

	LPCSTR act_str = NULL;
	CGameObject* GO = smart_cast<CGameObject*>(item);
	shared_str section_name = GO->cNameSect();

	//Custom Use action
	shared_str functor_field;
	LPCSTR functor_name = NULL;
	for (u8 i = 1; i <= 4; ++i)
	{
		functor_field.printf("use%d_functor", i);
		functor_name = READ_IF_EXISTS(pSettings, r_string, section_name, functor_field.c_str(), 0);
		if (functor_name)
		{
			luabind::functor<LPCSTR> funct;
			if (ai().script_engine().functor(functor_name, funct))
			{
				act_str = funct(GO->lua_game_object());
				if (act_str)
				{
					m_UIPropertiesBox->AddItem(act_str, NULL, INVENTORY_EAT2_ACTION + i - 1);
					b_show = true;
				}
			}
		}
	}
}

void CUIActorMenu::PropertiesBoxForPlaying(PIItem item, bool& b_show)
{
	CPda* pPda = smart_cast<CPda*>(item);
	if(!pPda || !pPda->CanPlayScriptFunction())
		return;

	LPCSTR act_str = "st_play";
	m_UIPropertiesBox->AddItem(act_str,  NULL, INVENTORY_PLAY_ACTION);
	b_show = true;
}

void CUIActorMenu::PropertiesBoxForDrop( CUICellItem* cell_item, PIItem item, bool& b_show )
{
	if (!item->IsQuestItem() && !ho_equipped)
	{
		if (item->parent_id() == m_pActorInvOwner->object_id())
		{
			m_UIPropertiesBox->AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
			if (cell_item->ChildsCount())
				m_UIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
		}
		else
			m_UIPropertiesBox->AddItem("st_take", NULL, INVENTORY_TO_BAG_ACTION);
		b_show = true;
	}
}

void CUIActorMenu::PropertiesBoxForRepair( PIItem item, bool& b_show )
{
	CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>( item );
	CWeapon*       pWeapon = smart_cast<CWeapon*>( item );
	CHelmet*       pHelmet = smart_cast<CHelmet*>( item );

	if ( (pOutfit || pWeapon || pHelmet) && item->GetCondition() < 0.99f )
	{
		m_UIPropertiesBox->AddItem( "ui_inv_repair", NULL, INVENTORY_REPAIR );
		b_show = true;
	}
}

//Alundaio: Ability to donate item during trade
void CUIActorMenu::PropertiesBoxForDonate(PIItem item, bool& b_show)
{
	if (!item->IsQuestItem())
	{
		m_UIPropertiesBox->AddItem("st_donate", NULL, INVENTORY_DONATE_ACTION);
		b_show = true;
	}
}
//-Alundaio

void CUIActorMenu::ProcessPropertiesBoxClicked( CUIWindow* w, void* d )
{
	PIItem			item		= CurrentIItem();
	CUICellItem*	cell_item	= CurrentItem();
	if ( !m_UIPropertiesBox->GetClickedItem() || !item || !cell_item || !cell_item->OwnerList() )
	{
		return;
	}
	CWeapon* weapon = smart_cast<CWeapon*>( item );

	switch (m_UIPropertiesBox->GetClickedItem()->GetTAG())
	{
	case INVENTORY_TO_HANDS_ACTION:
		ActivateItem(cell_item);
		break;
	case INVENTORY_TO_QUICK_ACTION:{
		u16 idx		= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy(ACTOR_DEFS::g_quick_use_slots[idx], *item->m_section_id);
		m_pInvPocket[idx]->ClearAll(true);
		InitPocket(idx);
		}break;
	case INVENTORY_FROM_QUICK_ACTION:{
		u16 idx		= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy(ACTOR_DEFS::g_quick_use_slots[GetPocketIdx(cell_item->OwnerList())], "");
		m_pInvPocket[idx]->ClearAll(true);
		InitPocket(idx);
		}break;
	case INVENTORY_TO_SLOT_ACTION:
		if (item->InHands())
			ActivateItem(cell_item, eItemPlaceSlot, item->BaseSlot());
		else
			ToSlot(cell_item, true, item->BaseSlot());
		break;
	case INVENTORY_TO_BAG_ACTION:
		if (item->InHands(), eItemPlaceRuck)
			ActivateItem(cell_item);
		else
			ToBag(cell_item, false);
		break;
	case INVENTORY_PICK_UP_ACTION:
		SendEvent_PickUpItem(item);
		break;
	case INVENTORY_DONATE_ACTION:
		DonateCurrentItem(cell_item);
		break;
	case INVENTORY_EAT_ACTION:
		TryUseItem(cell_item);
		break;
	case INVENTORY_EAT2_ACTION:
		{
			CGameObject* GO = smart_cast<CGameObject*>(item);
			LPCSTR functor_name = READ_IF_EXISTS(pSettings, r_string, GO->cNameSect(), "use1_action_functor", 0);
			if (functor_name)
			{
				luabind::functor<bool>	funct1;
				if (ai().script_engine().functor(functor_name, funct1))
				{
					if (funct1(GO->lua_game_object(), 1))
						TryUseItem(cell_item);
				}
			}
			break;
		}
	case INVENTORY_EAT3_ACTION:
		{
			CGameObject* GO = smart_cast<CGameObject*>(item);
			LPCSTR functor_name = READ_IF_EXISTS(pSettings, r_string, GO->cNameSect(), "use2_action_functor", 0);
			if (functor_name)
			{
				luabind::functor<bool>	funct2;
				if (ai().script_engine().functor(functor_name, funct2))
				{
					if (funct2(GO->lua_game_object(), 2))
						TryUseItem(cell_item);
				}
			}
			break;
		}
	case INVENTORY_EAT4_ACTION:
	{
		CGameObject* GO = smart_cast<CGameObject*>(item);
		LPCSTR functor_name = READ_IF_EXISTS(pSettings, r_string, GO->cNameSect(), "use3_action_functor", 0);
		if (functor_name)
		{
			luabind::functor<bool>	funct3;
			if (ai().script_engine().functor(functor_name, funct3))
			{
				if (funct3(GO->lua_game_object(), 3))
					TryUseItem(cell_item);
			}
		}
		break;
	}
	case INVENTORY_EAT5_ACTION:
	{
		CGameObject* GO = smart_cast<CGameObject*>(item);
		LPCSTR functor_name = READ_IF_EXISTS(pSettings, r_string, GO->cNameSect(), "use4_action_functor", 0);
		if (functor_name)
		{
			luabind::functor<bool>	funct4;
			if (ai().script_engine().functor(functor_name, funct4))
			{
				if (funct4(GO->lua_game_object(), 4))
					TryUseItem(cell_item);
			}
		}
		break;
	}
	case INVENTORY_DROP_ACTION:
		{
			void* d = m_UIPropertiesBox->GetClickedItem()->GetData();
			if (d == (void*)33)
				DropAllCurrentItem();
			else
				SendEvent_Item_Drop(item, m_pActorInvOwner->object_id());
			break;
		}
	case INVENTORY_ATTACH_ADDON:
		{
			PIItem item = CurrentIItem(); // temporary storing because of AttachAddon is setting curiitem to NULL
			AttachAddon((PIItem)(m_UIPropertiesBox->GetClickedItem()->GetData()));
			if (m_currMenuMode == mmDeadBodySearch)
				RemoveItemFromList(m_pDeadBodyBagList, item);
			else if (!item->object().H_Parent())
				RemoveItemFromList(m_pTrashList, item);
			break;
		}
	case INVENTORY_DETACH_SCOPE_ADDON:
		DetachAddon(weapon->GetScopeName().c_str());
		break;
	case INVENTORY_DETACH_SILENCER_ADDON:
		DetachAddon(weapon->GetSilencerName().c_str());
		break;
	case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
		DetachAddon(weapon->GetGrenadeLauncherName().c_str());
		break;
	case INVENTORY_RELOAD_MAGAZINE:
		if (weapon)
			weapon->Action( kWPN_RELOAD, CMD_START );
		break;
	case INVENTORY_UNLOAD_MAGAZINE:
		{
			CWeaponMagazined* weap_mag = smart_cast<CWeaponMagazined*>((CWeapon*)cell_item->m_pData);
			if (weap_mag)
			{
				weap_mag->UnloadMagazine();
				m_pActorInvOwner->inventory().Ruck(item);
			}
			break;
		}
	case INVENTORY_DISCHARGE_WEAPON:
		{
			CWeaponMagazined* weap_mag = smart_cast<CWeaponMagazined*>((CWeapon*)cell_item->m_pData);
			if (weap_mag)
			{
				weap_mag->Discharge();
				m_pActorInvOwner->inventory().Ruck(item);
			}
			break;
		}
	case INVENTORY_REPAIR:
		{
			TryRepairItem(this,0);
			return;
			break;
		}
	case INVENTORY_PLAY_ACTION:
		{
			CPda* pPda = smart_cast<CPda*>(item);
			if(!pPda)
				break;
			pPda->PlayScriptFunction();
			break;
		}
	}//switch

	//SetCurrentItem( NULL );
	UpdateItemsPlace();
	UpdateConditionProgressBars();
}//ProcessPropertiesBoxClicked

void CUIActorMenu::UpdateOutfit()
{
	CCustomOutfit* outfit	= m_pActorInvOwner->GetOutfit();
	m_HelmetOver->Show		((outfit && !outfit->bIsHelmetAvaliable));
}

void CUIActorMenu::RefreshCurrentItemCell()
{
	CUICellItem* ci = CurrentItem();
	if (!ci)
		return;

	if (ci->ChildsCount() > 0)
	{
		CUIDragDropListEx* invlist = GetListByType(iActorBag);
			
		if (invlist->IsOwner(ci))
		{
			CUICellItem* parent = invlist->RemoveItem(ci, true);

			while (parent->ChildsCount())
			{
				CUICellItem* child = parent->PopChild(NULL);
				invlist->SetItem(child);
			}

			invlist->SetItem(parent, GetUICursor().GetCursorPosition());
		}
	}
}
