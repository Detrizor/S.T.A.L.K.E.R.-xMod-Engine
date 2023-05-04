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

#include "../grenadelauncher.h"
#include "../Artefact.h"
#include "../eatable_item.h"
#include "../BottleItem.h"
#include "../WeaponMagazined.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../UICursor.h"
#include "../MPPlayersBag.h"
#include "../player_hud.h"
#include "../CustomDetector.h"
#include "../PDA.h"
#include "../ActorBackpack.h"
#include "../actor_defs.h"
#include "../item_container.h"

#include "addon.h"
#include "string_table.h"

using namespace ::luabind; //Alundaio

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
		m_pPocketOver[i]->Show		(!m_pActorInv->PocketPresent(i));
	}

	m_pTrashList->Show					(true);

	InitInventoryContents				();
}

void CUIActorMenu::UpdatePocketsPresence()
{
	for (u16 i = 0; i < m_pockets_count; i++)
		m_pPocketOver[i]->Show(!m_pActorInv->PocketPresent(i));
}

void CUIActorMenu::DeInitInventoryMode()
{
}

void CUIActorMenu::SendEvent_PickUpItem(PIItem pItem, u16 type, u16 slot_id)
{
	pItem->m_ItemCurrPlaceBackup			= pItem->m_ItemCurrPlace;
	pItem->m_ItemCurrPlaceBackup.type		= type;
	pItem->m_ItemCurrPlaceBackup.slot_id	= slot_id;
	pItem->Transfer							(m_pActorInvOwner->object_id());
}

void CUIActorMenu::SendEvent_Item_Eat(PIItem pItem, u16 recipient)
{
	if(pItem->parent_id()!=recipient)
		pItem->Transfer(recipient);

	NET_Packet						P;
	CGameObject::u_EventGen			(P, GEG_PLAYER_ITEM_EAT, recipient);
	P.w_u16							(pItem->object().ID());
	CGameObject::u_EventSend		(P);
}

void CUIActorMenu::DropAllCurrentItem()
{
	if (CurrentIItem())
	{
		u32 const cnt			= CurrentItem()->ChildsCount();
		for (u32 i = 0; i < cnt; ++i)
		{
			CUICellItem* itm	= CurrentItem()->PopChild(NULL);
			PIItem item			= (PIItem)itm->m_pData;
			item->Transfer		();
		}
		CurrentIItem()->Transfer();
	}
	SetCurrentItem				(NULL);
}

bool CUIActorMenu::DropAllItemsFromRuck( bool quest_force )
{
	if (!IsShown() || !m_pInventoryBagList || m_currMenuMode != mmInventory)
		return					false;

	u32 const ci_count			= m_pInventoryBagList->ItemsCount();
	for (u32 i = 0; i < ci_count; ++i)
	{
		CUICellItem* ci			= m_pInventoryBagList->GetItemIdx(i);
		VERIFY					(ci);
		PIItem item				= (PIItem)ci->m_pData;
		VERIFY					(item);

		u32 const cnt			= ci->ChildsCount();
		for (u32 j = 0; j < cnt; ++j)
		{
			CUICellItem*		child_ci   = ci->PopChild(NULL);
			PIItem				child_item = (PIItem)child_ci->m_pData;
			child_item->Transfer();
		}
		item->Transfer			();
	}
	SetCurrentItem				(NULL);
	return						true;
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

void CUIActorMenu::OnInventoryAction(PIItem pItem, bool take, u8 zone)
{
	CUIDragDropListEx* lst_to_check		= NULL;
	if (zone == 1)
		lst_to_check					= m_pTradeActorList;
	else if (zone == 2)
		lst_to_check					= m_pDeadBodyBagList;
	else if (zone == 3)
		lst_to_check					= GetListByType(iActorBag);

	if (take)
	{
		bool b_already					= false;
		bool b_deleted					= false;
		CUIDragDropListEx* lst_to_add	= NULL;
		if (zone == 1)
		{
			SInvItemPlace pl			= pItem->m_ItemCurrPlace;
			if (pl.type == eItemPlaceSlot)
				lst_to_add				= GetSlotList(pl.slot_id);
			else if (pl.type == eItemPlacePocket)
				lst_to_add				= m_pInvPocket[pl.slot_id];
		}
		else if (zone == 2)
			lst_to_add					= m_pDeadBodyBagList;
		else if (zone == 3)
			lst_to_add					= GetListByType(iActorBag);

		CUICellItem* ci					= NULL;
		if (FindItemInList(lst_to_check, pItem, ci))
		{
			if (lst_to_check == lst_to_add)
				b_already				= true;
			else
				b_deleted				= RemoveItemFromList(lst_to_check, pItem);
		}
		RemoveItemFromList				(m_pTrashList, pItem);

		if (zone == 1 && !(b_already && b_deleted))
		{
			for (u8 i = 1; i <= m_slot_count; ++i)
			{
				CUIDragDropListEx* curr	= m_pInvList[i];
				if (curr)
				{
					CUICellItem* ci		= NULL;
					if (FindItemInList(curr, pItem, ci))
					{
						if (curr == lst_to_add)
							b_already	= true;
						else
							RemoveItemFromList(curr, pItem);
					}
				}
			}
		}
				
		if (zone == 1 && !(b_already && b_deleted))
		{
			for (u8 i = 0; i < m_pockets_count; i++)
			{
				CUIDragDropListEx* pocket	= m_pInvPocket[i];
				CUICellItem* ci			= NULL;
				if (FindItemInList(pocket, pItem, ci))
				{
					if (pocket == lst_to_add)
						b_already		= true;
					else
						RemoveItemFromList(pocket, pItem);
				}
			}
		}

		if (!b_already && lst_to_add)
		{
			CUICellItem* itm			= create_cell_item(pItem);
			if (!lst_to_add->CanSetItem(itm))
				lst_to_add->RemoveItem	(lst_to_add->GetItemIdx(0), true);
			lst_to_add->SetItem			(itm);
			if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
				ColorizeItem			(itm);
		}
	}
	else
	{
		CUIDragDropListEx* _drag_owner	= NULL;
		if (CUIDragDropListEx::m_drag_item)
		{
			CUIInventoryCellItem* ici	= smart_cast<CUIInventoryCellItem*>(CUIDragDropListEx::m_drag_item->ParentItem());
			R_ASSERT					(ici);
			if (ici->object() == pItem)
				_drag_owner				= ici->OwnerList();
		}
				
		if (lst_to_check == _drag_owner)
			lst_to_check->DestroyDragItem();

		if (!RemoveItemFromList(lst_to_check, pItem) && zone == 1)
		{
			for (u8 i = 1; i <= m_slot_count; ++i)
			{
				CUIDragDropListEx* curr	= m_pInvList[i];
				if (curr)
				{
					if (curr == _drag_owner)
						curr->DestroyDragItem();
					if (RemoveItemFromList(curr, pItem))
						goto _finish;
				}
			}

			for (u8 i = 0; i < m_pockets_count; i++)
			{
				CUIDragDropListEx* pocket	= m_pInvPocket[i];
				if (pocket == _drag_owner)
					pocket->DestroyDragItem();
				if (RemoveItemFromList(pocket, pItem))
					break;
			}
		}
	}

_finish:
	UpdateItemsPlace();
	UpdateConditionProgressBars();
}

void CUIActorMenu::InitCellForSlot( u16 slot_idx )
{
	PIItem item	= m_pActorInv->ItemFromSlot		(slot_idx);
	if (!item)
		return;

	CUIDragDropListEx* curr_list				= GetSlotList( slot_idx );
	if (!curr_list)
		return;

	CUICellItem* cell_item						= create_cell_item(item);
	curr_list->SetItem							(cell_item);
	if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
		ColorizeItem							(cell_item);
}

void CUIActorMenu::InitPocket(u16 pocket_idx)
{
	TIItemContainer		pocket_list;
	pocket_list			= m_pActorInv->m_pockets[pocket_idx];
	::std::sort(pocket_list.begin(), pocket_list.end(), InventoryUtilities::GreaterRoomInRuck);

	LPCSTR sect										= ACTOR_DEFS::g_quick_use_slots[pocket_idx];
	for (TIItemContainer::iterator itb = pocket_list.begin(), ite = pocket_list.end(); itb != ite; ++itb)
	{
		if ((*itb)->Section(true) == sect)
		{
			CUICellItem* itm						= create_cell_item(*itb);
			m_pInvPocket[pocket_idx]->SetItem		(itm);
			if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
				ColorizeItem						(itm);
		}
	}


	for (TIItemContainer::iterator itb = pocket_list.begin(), ite = pocket_list.end(); itb != ite; ++itb)
	{
		if (smart_cast<CMPPlayersBag*>(&(*itb)->object()) || (*itb)->Section(true) == sect)
			continue;

		CUICellItem* itm						= create_cell_item(*itb);
		m_pInvPocket[pocket_idx]->SetItem		(itm);
		if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
			ColorizeItem						(itm);
	}
}

void CUIActorMenu::InitInventoryContents()
{
	ClearAllLists						();
	m_pMouseCapturer					= NULL;
	m_UIPropertiesBox->Hide				();
	SetCurrentItem						(NULL);

	//Slots
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		if (m_pInvList[i])
			InitCellForSlot				(i);
		else if (!m_pActorInv->SlotIsPersistent(i))
			InitCellForSlot				(i);
	}
	
	for (u8 i = 0; i < m_pockets_count; i++)
		InitPocket						(i);
}

extern bool TryCustomUse(PIItem item);
bool CUIActorMenu::ToSlot(CUICellItem* itm, u16 slot_id, bool assume_alternative)
{
	PIItem item									= (PIItem)itm->m_pData;
	if (slot_id == OUTFIT_SLOT || slot_id == HELMET_SLOT || item->CurrSlot() == OUTFIT_SLOT || item->CurrSlot() == HELMET_SLOT)
		return									false;

	bool already								= item == m_pActorInv->ItemFromSlot(slot_id);
	bool own									= item->parent_id() == m_pActorInvOwner->object_id();
	if (m_pActorInv->CanPutInSlot(item, slot_id) || already)
	{
		if (item == m_pActorInv->ActiveItem() || item == m_pActorInv->LeftItem())
			m_pActorInv->ActivateItem			(item, eItemPlaceSlot, slot_id);
		else
		{
			if (slot_id == item->HandSlot())
			{
				m_pActorInv->ActivateItem		(item);
				if (!own)
					ToRuck						(item);
			}
			else
			{
				CUIDragDropListEx* old_owner	= itm->OwnerList();
				CUIDragDropListEx* new_owner	= GetSlotList(slot_id);
				CUICellItem* i					= old_owner->RemoveItem(itm, old_owner == new_owner);
				new_owner->SetItem				(i);

				if (own)
					m_pActorInv->Slot			(slot_id, item);
				else
					SendEvent_PickUpItem		(item, eItemPlaceSlot, slot_id);
			}
		}

		return									true;
	}
	else if (assume_alternative)
	{
		if (slot_id == PRIMARY_SLOT)
		{
			if (m_pActorInv->CanPutInSlot(item, SECONDARY_SLOT))
				return ToSlot					(itm, SECONDARY_SLOT);
		}
		else if (slot_id == SECONDARY_SLOT)
		{
			if (m_pActorInv->CanPutInSlot(item, PRIMARY_SLOT))
				return ToSlot					(itm, PRIMARY_SLOT);
		}
	}

	PIItem _item								= m_pActorInv->ItemFromSlot(slot_id);
	if (item == m_pActorInv->ActiveItem() || item == m_pActorInv->LeftItem())
		m_pActorInv->ActivateItem				(_item, eItemPlaceSlot, slot_id);
	else if (slot_id == item->HandSlot() && !TryCustomUse(item) && _item && _item->BaseSlot() && _item->BaseSlot() == item->BaseSlot())
		m_pActorInv->ActivateItem				(item, item->CurrPlace(), item->CurrPlace() == eItemPlaceSlot ? item->CurrSlot() : item->CurrPocket());
	else
		return									false;
	if (!own)
		ToRuck									(item);
	return										true;
}

bool CUIActorMenu::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem item							= (PIItem)itm->m_pData;
	bool own							= item->parent_id() == GetBag()->O.ID();
	if (!GetBag()->CanTakeItem(item) && !own)
		return							false;

	CUIDragDropListEx* old_owner		= itm->OwnerList();
	CUIDragDropListEx* new_owner		= (b_use_cursor_pos) ? CUIDragDropListEx::m_drag_item->BackList() : GetListByType(iActorBag);
	CUICellItem* i						= old_owner->RemoveItem(itm, old_owner == new_owner);
	(b_use_cursor_pos) ?				new_owner->SetItem(i, old_owner->GetDragItemPosition()) : new_owner->SetItem(i);

	if (!own)
		item->Transfer					(GetBag()->O.ID());

	if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
		ColorizeItem					(itm);

	return								true;
}

bool CUIActorMenu::ToPocket(CUICellItem* itm, bool b_use_cursor_pos, u16 pocket_id)
{
	PIItem item							= (PIItem)itm->m_pData;
	bool own							= ::std::find(m_pActorInv->m_pockets[pocket_id].begin(), m_pActorInv->m_pockets[pocket_id].end(), item) != m_pActorInv->m_pockets[pocket_id].end();
	if (!m_pActorInv->CanPutInPocket(item, pocket_id) && !own)
		return							false;

	if (item == m_pActorInv->ActiveItem() || item == m_pActorInv->LeftItem())
	{
		m_pActorInv->ActivateItem		(item, eItemPlacePocket, pocket_id);
		return							true;
	}

	CUIDragDropListEx* old_owner		= itm->OwnerList();
	CUIDragDropListEx* new_owner		= m_pInvPocket[pocket_id];
	CUICellItem* i						= old_owner->RemoveItem(itm, old_owner == new_owner);
	(b_use_cursor_pos) ?				new_owner->SetItem(i, old_owner->GetDragItemPosition()) : new_owner->SetItem(i);

	if (!own)
	{
		if (item->parent_id() == m_pActorInvOwner->object_id())
			m_pActorInv->Pocket			(item, pocket_id);
		else
			SendEvent_PickUpItem		(item, eItemPlacePocket, pocket_id);
	}

	if (m_currMenuMode == mmTrade && m_pPartnerInvOwner)
		ColorizeItem					(itm);

	return								true;
}

void CUIActorMenu::ToRuck(PIItem item)
{
	if (item->parent_id() == m_pActorInvOwner->object_id())
		m_pActorInv->Ruck				(item);
	else
		SendEvent_PickUpItem			(item, eItemPlaceRuck);
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
	
	CEatableItem*	pEatableItem	= item->cast<CEatableItem*>();

	if (!pEatableItem)
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
	if (m_currMenuMode != mmTrade && m_pActorInv->CanPutInSlot(item, item->HandSlot()) && !ho_equipped)
	{
		m_UIPropertiesBox->AddItem		("st_to_hands", NULL, INVENTORY_TO_HANDS_ACTION);
		b_show							= true;
	}

	CUIDragDropListEx* list					= cell_item->OwnerList();
	if (GetListType(list) == iActorPocket)
	{
		u16 idx								= GetPocketIdx(list);
		if (item->Section(true) == ACTOR_DEFS::g_quick_use_slots[idx])
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
	u16 base_slot					= item->BaseSlot();

	if (!ho && base_slot != NO_ACTIVE_SLOT && m_pActorInv->CanPutInSlot(item, base_slot))
	{
		m_UIPropertiesBox->AddItem		("st_move_to_slot", NULL, INVENTORY_TO_SLOT_ACTION);
		b_show							= true;
	}

	if (item->Ruck() && m_pActorInv->CanPutInRuck(item) && (item->CurrSlot() != NO_ACTIVE_SLOT || m_currMenuMode == mmDeadBodySearch) && !ho_equipped)
	{
		m_UIPropertiesBox->AddItem		("st_move_to_ruck", NULL, INVENTORY_TO_BAG_ACTION);
		b_show							= true;
	}
}

shared_str attach_to	= "st_attach_to";
shared_str dettach		= "st_detach";

void CUIActorMenu::PropertiesBoxForWeapon(CUICellItem* cell_item, PIItem item, bool& b_show)
{
	if (!item->InHands())
		return;

	//отсоединение аддонов от вещи
	CAddonOwner* ao = item->cast<CAddonOwner*>();
	if (!ao)
		return;

	for (const auto slot : ao->AddonSlots())
	{
		if (slot->addon)
		{
			LPCSTR title				= *shared_str().printf("%s %s (%s)", *CStringTable().translate(dettach), slot->addon->NameShort(), *CStringTable().translate(slot->name));
			m_UIPropertiesBox->AddItem	(title, (void*)slot->addon, INVENTORY_DETACH_ADDON);
			b_show = true;
		}
	}
}

void CUIActorMenu::PropertiesBoxForAddon(PIItem item, bool& b_show)
{
	PIItem active_item					= m_pActorInv->ActiveItem();
	if (!active_item)
		return;
	CAddonOwner* ao						= active_item->cast<CAddonOwner*>();
	if (!ao)
		return;

	CAddon* addon						= item->cast<CAddon*>();
	if (!addon)
		return;

	for (const auto slot : ao->AddonSlots())
	{
		if (slot->CanTake(addon))
		{
			LPCSTR title				= *shared_str().printf("%s %s (%s)", *CStringTable().translate(attach_to), active_item->NameShort(), *CStringTable().translate(slot->name));
			m_UIPropertiesBox->AddItem	(title, (void*)slot, INVENTORY_ATTACH_ADDON);
			b_show						= true;
		}
	}
}

void CUIActorMenu::PropertiesBoxForUsing(PIItem item, bool& b_show)
{
	LPCSTR title_str			= NULL;
	CGameObject* GO				= smart_cast<CGameObject*>(item);
	LPCSTR section_name			= *GO->cNameSect();

	//Custom Use action
	shared_str								functor_field;
	LPCSTR functor_name						= NULL;
	for (u8 i = 1; i <= 4; ++i)
	{
		functor_field.printf				("use%d_query_functor", i);
		functor_name						= READ_IF_EXISTS(pSettings, r_string, section_name, functor_field.c_str(), 0);
		if (functor_name)
		{
			::luabind::functor<bool>		funct;
			if (ai().script_engine().functor(functor_name, funct) && funct(GO->lua_game_object(), i))
			{
				functor_field.printf		("use%d_title", i);
				title_str					= pSettings->r_string(section_name, *functor_field);
				m_UIPropertiesBox->AddItem	(title_str, NULL, INVENTORY_EAT2_ACTION + i - 1);
				b_show						= true;
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
	if (!ho_equipped)
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

	if ((pOutfit || pWeapon || pHelmet) && item->GetCondition() < 1.f - EPS)
	{
		m_UIPropertiesBox->AddItem( "ui_inv_repair", NULL, INVENTORY_REPAIR );
		b_show = true;
	}
}

//Alundaio: Ability to donate item during trade
void CUIActorMenu::PropertiesBoxForDonate(PIItem item, bool& b_show)
{
	m_UIPropertiesBox->AddItem("st_donate", NULL, INVENTORY_DONATE_ACTION);
	b_show = true;
}
//-Alundaio

void CUIActorMenu::ProcessPropertiesBoxClicked(CUIWindow* w, void* d)
{
	PIItem			item		= CurrentIItem();
	CUICellItem*	cell_item	= CurrentItem();
	if (!m_UIPropertiesBox->GetClickedItem() || !item || !cell_item || !cell_item->OwnerList())
		return;

	switch (m_UIPropertiesBox->GetClickedItem()->GetTAG())
	{
	case INVENTORY_TO_HANDS_ACTION:
		ToSlot							(cell_item, item->HandSlot());
		break;
	case INVENTORY_TO_QUICK_ACTION:{
		u16 idx							= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy						(ACTOR_DEFS::g_quick_use_slots[idx], *item->Section(true));
		m_pInvPocket[idx]->ClearAll		(true);
		InitPocket						(idx);
		}break;
	case INVENTORY_FROM_QUICK_ACTION:{
		u16 idx							= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy						(ACTOR_DEFS::g_quick_use_slots[GetPocketIdx(cell_item->OwnerList())], "");
		m_pInvPocket[idx]->ClearAll		(true);
		InitPocket						(idx);
		}break;
	case INVENTORY_TO_SLOT_ACTION:
		ToSlot							(cell_item, item->BaseSlot(), true);
		break;
	case INVENTORY_TO_BAG_ACTION:
		ToBag							(cell_item, false);
		break;
	case INVENTORY_PICK_UP_ACTION:
		SendEvent_PickUpItem			(item);
		break;
	case INVENTORY_DONATE_ACTION:
		DonateCurrentItem				(cell_item);
		break;
	case INVENTORY_EAT_ACTION:
		TryUseItem						(cell_item);
		break;
	case INVENTORY_EAT2_ACTION:
	case INVENTORY_EAT3_ACTION:
	case INVENTORY_EAT4_ACTION:
	case INVENTORY_EAT5_ACTION:
	{
		CGameObject* GO					= smart_cast<CGameObject*>(item);
		shared_str						functor_str;
		u32 num							= m_UIPropertiesBox->GetClickedItem()->GetTAG() - INVENTORY_EAT2_ACTION + 1;
		functor_str.printf				("use%d_action_functor", num);
		functor_str						= pSettings->r_string(GO->cNameSect(), *functor_str);
		::luabind::functor<bool>		funct;
		ai().script_engine().functor	(*functor_str, funct);
		funct							(GO->lua_game_object(), num);
		break;
	}
	case INVENTORY_DROP_ACTION:{
		void* d							= m_UIPropertiesBox->GetClickedItem()->GetData();
		if (d == (void*)33)
			DropAllCurrentItem			();
		else
			item->Transfer				();
		}break;
	case INVENTORY_ATTACH_ADDON:
	{
		CAddonOwner* ao					= m_pActorInv->ActiveItem()->cast<CAddonOwner*>();
		SAddonSlot* slot				= (SAddonSlot*)m_UIPropertiesBox->GetClickedItem()->GetData();
		AttachAddon						(ao, item->cast<CAddon*>(), slot->idx);
		break;
	}
	case INVENTORY_DETACH_ADDON:
	{
		CAddonOwner* ao					= item->cast<CAddonOwner*>();
		DetachAddon						(ao, (CAddon*)m_UIPropertiesBox->GetClickedItem()->GetData());
		break;
	}
	case INVENTORY_REPAIR:
		TryRepairItem					(this, 0);
		break;
	case INVENTORY_PLAY_ACTION:{
		CPda* pPda						= smart_cast<CPda*>(item);
		if (pPda)
			pPda->PlayScriptFunction	();
		}break;
	}

	UpdateItemsPlace();
	UpdateConditionProgressBars();
}

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

bool CUIActorMenu::AttachAddon(CAddonOwner* ao, CAddon* addon, u16 slot)
{
	int res								= ao->AttachAddon(addon, slot);
	if (res == 2)
	{
		PlaySnd							(eAttachAddon);
		if (IsShown())
			HideDialog					();
	}
	return								!!res;
}

void CUIActorMenu::DetachAddon(CAddonOwner* ao, CAddon* addon)
{
	if (ao->DetachAddon(addon) == 2)
	{
		PlaySnd							(eDetachAddon);
		HideDialog						();
	}
}
