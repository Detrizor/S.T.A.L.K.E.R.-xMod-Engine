#include "stdafx.h"
#include "UIActorMenu.h"
#include "../inventory.h"
#include "../inventoryOwner.h"
#include "UIInventoryUtilities.h"
#include "UIItemInfo.h"
#include "../Level.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICellCustomItems.h"
#include "UIItemInfo.h"
#include "UIFrameLineWnd.h"
#include "UIPropertiesBox.h"
#include "UIListBoxItem.h"
#include "UIMainIngameWnd.h"
#include "UIGameCustom.h"
#include "weaponBM16.h"
#include "WeaponAmmo.h"
#include "UIProgressBar.h"

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
#include "item_usable.h"

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

void CUIActorMenu::SendEvent_Item_Eat(PIItem pItem, u16 recipient)
{
	if(pItem->parent_id()!=recipient)
		pItem->O.transfer			(recipient);

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
			CUICellItem* itm	= CurrentItem()->PopChild(nullptr);
			PIItem item			= (PIItem)itm->m_pData;
			item->O.transfer	();
		}
		CurrentIItem()->O.transfer();
	}
	SetCurrentItem				(nullptr);
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
			CUICellItem*		child_ci   = ci->PopChild(nullptr);
			PIItem				child_item = (PIItem)child_ci->m_pData;
			child_item->O.transfer();
		}
		item->O.transfer		();
	}
	SetCurrentItem				(nullptr);
	return						true;
}

void CUIActorMenu::InitCellForSlot(u16 slot_idx)
{
	if (auto curr_list = GetSlotList(slot_idx))
	{
		if (auto item = m_pActorInv->ItemFromSlot(slot_idx))
		{
			CUICellItem* cell_item		= item->getIcon();
			curr_list->SetItem			(cell_item);
			ColorizeItem				(cell_item);

			if (auto bar = m_pInvSlotProgress[slot_idx])
				bar->SetProgressPos		((item) ? item->GetCondition() : 0.f);

			UpdateOutfit				();
		}

		curr_list->setValid				(true);
	}
}

void CUIActorMenu::InitPocket(u16 pocket_idx)
{
	auto& pocket_list					= m_pInvPocket[pocket_idx];
	TIItemContainer items_list			= m_pActorInv->m_pockets[pocket_idx];
	items_list.sort						(InventoryUtilities::GreaterRoomInRuck);

	shared_str sect						= ACTOR_DEFS::g_quick_use_slots[pocket_idx];
	for (auto& item : items_list)
	{
		if (item->Section(true) == sect)
		{
			CUICellItem* itm			= item->getIcon();
			pocket_list->SetItem		(itm);
			ColorizeItem				(itm);
		}
	}

	for (auto& item : items_list)
	{
		if (item->Section(true) != sect)
		{
			CUICellItem* itm			= item->getIcon();
			pocket_list->SetItem		(itm);
			ColorizeItem				(itm);
		}
	}
	
	auto& label							= m_pPocketInfo[pocket_idx];
	if (m_pActorInvOwner->inventory().PocketPresent(pocket_idx))
	{
		shared_str						str;
		str.printf						("quick_use_str_%d", pocket_idx + 1);
		str								= CStringTable().translate(str);
		float volume					= CalcItemsVolume(pocket_list);
		float max_volume				= m_pActorInvOwner->GetOutfit()->m_pockets[pocket_idx];
		LPCSTR li_str					= CStringTable().translate("st_li").c_str();
		str.printf						("(%s) %.2f/%.2f %s", str.c_str(), volume, max_volume, li_str);
		label->SetText					(str.c_str());
	}
	else
		label->SetText					("");
	
	for (int i = 0, count = pocket_list->ItemsCount(); i < count; ++i)
	{
		CUICellItem* ci					= pocket_list->GetItemIdx(i);
		PIItem item						= static_cast<PIItem>(ci->m_pData);
		ci->m_select_equipped			= (item->Section(true) == ACTOR_DEFS::g_quick_use_slots[pocket_idx]);
	}

	pocket_list->setValid				(true);
}

void CUIActorMenu::InitInventoryContents()
{
	ClearAllLists						();
	m_pMouseCapturer					= nullptr;
	m_UIPropertiesBox->Hide				();
	SetCurrentItem						(nullptr);

	//Slots
	for (u8 i = 1; i <= m_slot_count; ++i)
		if (m_pInvList[i] || !m_pActorInv->SlotIsPersistent(i))
			InitCellForSlot				(i);
	
	for (u8 i = 0; i < m_pockets_count; ++i)
		InitPocket						(i);

	init_vicinity						();
}

bool CUIActorMenu::ToSlot(CUICellItem* itm, u16 slot_id, bool assume_alternative)
{
	PIItem item							= static_cast<PIItem>(itm->m_pData);
	PIItem item_in_slot					= m_pActorInv->ItemFromSlot(slot_id);
	bool own							= (item->parent_id() == m_pActorInvOwner->object_id());

	if (m_pActorInv->CanPutInSlot(item, slot_id))
	{
		if (m_pActorInv->InHands(item))
			m_pActorInv->ActivateItem	(item, eItemPlaceSlot, slot_id);
		else
		{
			if (slot_id == item->HandSlot())
			{
				m_pActorInv->ActivateItem(item);
				if (!own)
					ToRuck				(item);
			}
			else
			{
				if (own)
					m_pActorInv->Slot	(slot_id, item);
				else
					pickup_item			(item, eItemPlaceSlot, slot_id);
			}
		}

		return							true;
	}
	else if (assume_alternative)
	{
		if (slot_id == PRIMARY_SLOT)
		{
			if (m_pActorInv->CanPutInSlot(item, SECONDARY_SLOT))
				return ToSlot			(itm, SECONDARY_SLOT);
		}
		else if (slot_id == SECONDARY_SLOT)
		{
			if (m_pActorInv->CanPutInSlot(item, PRIMARY_SLOT))
				return ToSlot			(itm, PRIMARY_SLOT);
		}
	}

	if (m_pActorInv->InHands(item))
		m_pActorInv->ActivateItem		(item_in_slot, eItemPlaceSlot, slot_id);
	else if (slot_id == item->HandSlot() && item_in_slot && item->CurrSlot() && item_in_slot->BaseSlot() == item->BaseSlot())
		m_pActorInv->ActivateItem		(item, eItemPlaceSlot, item->CurrSlot());
	else
		return							false;

	if (!own)
		ToRuck							(item);

	return								true;
}

bool CUIActorMenu::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem item							= static_cast<PIItem>(itm->m_pData);
	if (item->parent_id() != m_pBag->O.ID() && m_pBag->CanTakeItem(item))
	{
		item->O.transfer				(GetBag()->O.ID());
		return							true;
	}
	return								false;
}

bool CUIActorMenu::ToPocket(CUICellItem* itm, bool b_use_cursor_pos, u16 pocket_id)
{
	PIItem item							= static_cast<PIItem>(itm->m_pData);
	if (!m_pActorInv->m_pockets[pocket_id].contains(item) && m_pActorInv->CanPutInPocket(item, pocket_id))
	{
		if (m_pActorInv->InHands(item))
			m_pActorInv->ActivateItem	(item, eItemPlacePocket, pocket_id);
		else if (item->parent_id() == m_pActorInvOwner->object_id())
			m_pActorInv->Pocket			(item, pocket_id);
		else
			pickup_item					(item, eItemPlacePocket, pocket_id);
		return							true;
	}
	return								false;
}

void CUIActorMenu::ToRuck(PIItem item)
{
	if (item->parent_id() == m_pActorInvOwner->object_id())
		m_pActorInv->Ruck				(item);
	else
		pickup_item						(item, eItemPlaceRuck);
}

CUIDragDropListEx* CUIActorMenu::GetSlotList(u16 slot_idx)
{
	if (slot_idx == NO_ACTIVE_SLOT || slot_idx >= m_pInvList.size())
		return nullptr;

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

	if (!item->O.scast<CEatableItem*>())
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
	
	if (m_currMenuMode != mmTrade && m_pActorInv->CanPutInSlot(item, item->HandSlot()))
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

	if (m_pActorInv->CanPutInSlot(item, base_slot))
	{
		m_UIPropertiesBox->AddItem		("st_move_to_slot", NULL, INVENTORY_TO_SLOT_ACTION);
		b_show							= true;
	}

	if ((item->CurrSlot() != NO_ACTIVE_SLOT || m_currMenuMode == mmDeadBodySearch) && !item->isGear(true))
	{
		m_UIPropertiesBox->AddItem		("st_move_to_ruck", NULL, INVENTORY_TO_BAG_ACTION);
		b_show							= true;
	}
}

static void process_ao_for_attach(MAddonOwner CPC ao, MAddon CPC addon, CUIPropertiesBox* pb, bool& b_show, LPCSTR str)
{
	shared_str							attach_str;
	for (auto& s : ao->AddonSlots())
	{
		if (s->canTake(addon, true))
		{
			attach_str.printf			("%s %s", str, *s->name);
			if (!s->steps && s->addons.size())
				attach_str.printf		("%s (%s %s)", attach_str.c_str(), CStringTable().translate("st_swap").c_str(), s->addons.front()->I->getNameShort());
			pb->AddItem					(*attach_str, static_cast<void*>(s.get()), INVENTORY_ADDON_ATTACH);
			b_show						= true;

			if (auto wpn = ao->O.scast<CWeaponBM16*>())
			{
				if (wpn->checkSecondChamber(s.get()))
				{
					auto ammo			= addon->O.scast<CWeaponAmmo*>();
					if (ammo->GetAmmoCount() >= 2)
					{
						attach_str.printf("%s %s", str, CStringTable().translate("st_both_chambers").c_str());
						pb->AddItem		(attach_str.c_str(), static_cast<void*>(s.get()), INVENTORY_ADDON_ATTACH_BOTH_CHAMBERS);
					}
				}
			}
		}
		
		for (auto a : s->addons)
			if (auto addon_ao = a->O.getModule<MAddonOwner>())
				process_ao_for_attach	(addon_ao, addon, pb, b_show, shared_str().printf("%s %s -", str, a->I->getNameShort()).c_str());
	}
}

void CUIActorMenu::PropertiesBoxForAddon(PIItem item, bool& b_show)
{
	if (auto addon = item->O.getModule<MAddon>())
		if (auto active_item = m_pActorInv->ActiveItem())
			if (auto ao = active_item->O.getModule<MAddonOwner>())
				process_ao_for_attach	(ao, addon, m_UIPropertiesBox, b_show,
					shared_str().printf("%s %s -", CStringTable().translate("st_attach_to").c_str(), active_item->getNameShort()).c_str());
}

void CUIActorMenu::PropertiesBoxForUsing(PIItem item, bool& b_show)
{
	if (auto usable = item->O.getModule<MUsable>())
	{
		int i							= 0;
		while (auto action = usable->getAction(++i))
		{
			if (action->performQueryFunctor())
			{
				m_UIPropertiesBox->AddItem(action->title.c_str(), reinterpret_cast<void*>(i), INVENTORY_CUSTOM_ACTION);
				b_show					= true;
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
	m_UIPropertiesBox->AddItem(act_str, NULL, INVENTORY_PLAY_ACTION);
	b_show = true;
}

void CUIActorMenu::PropertiesBoxForDrop(CUICellItem* cell_item, PIItem item, bool& b_show)
{
	if (!item->isGear(true))
	{
		if (item->parent_id() == m_pActorInvOwner->object_id())
		{
			m_UIPropertiesBox->AddItem	("st_drop", NULL, INVENTORY_DROP_ACTION);
			if (cell_item->ChildsCount())
				m_UIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
		}
		else
			m_UIPropertiesBox->AddItem	("st_take", NULL, INVENTORY_TO_BAG_ACTION);
		b_show							= true;
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
	PIItem item							= CurrentIItem();
	CUICellItem* cell_item				= CurrentItem();
	if (!m_UIPropertiesBox->GetClickedItem() || !item || !cell_item || !cell_item->OwnerList())
		return;

	switch (auto tag = m_UIPropertiesBox->GetClickedItem()->GetTAG())
	{
	case INVENTORY_TO_HANDS_ACTION:
		ToSlot							(cell_item, item->HandSlot());
		break;
	case INVENTORY_TO_QUICK_ACTION:
	{
		u16 idx							= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy						(ACTOR_DEFS::g_quick_use_slots[idx], *item->Section(true));
		m_pInvPocket[idx]->ClearAll		(true);
		InitPocket						(idx);
		break;
	}
	case INVENTORY_FROM_QUICK_ACTION:
	{
		u16 idx							= GetPocketIdx(cell_item->OwnerList());
		xr_strcpy						(ACTOR_DEFS::g_quick_use_slots[GetPocketIdx(cell_item->OwnerList())], "");
		m_pInvPocket[idx]->ClearAll		(true);
		InitPocket						(idx);
		break;
	}
	case INVENTORY_TO_SLOT_ACTION:
		ToSlot							(cell_item, item->BaseSlot(), true);
		break;
	case INVENTORY_TO_BAG_ACTION:
		ToRuck							(item);
		break;
	case INVENTORY_DONATE_ACTION:
		DonateCurrentItem				(cell_item);
		break;
	case INVENTORY_EAT_ACTION:
		TryUseItem						(cell_item);
		break;
	case INVENTORY_CUSTOM_ACTION:
	{
		int num							= reinterpret_cast<int>(m_UIPropertiesBox->GetClickedItem()->GetData());
		auto usable						= item->O.getModule<MUsable>();
		usable->performAction			(num, true);
		break;
	}
	case INVENTORY_DROP_ACTION:
	{
		void* d							= m_UIPropertiesBox->GetClickedItem()->GetData();
		if (d == (void*)33)
			DropAllCurrentItem			();
		else
			item->O.transfer			();
		break;
	}
	case INVENTORY_ADDON_ATTACH:
	case INVENTORY_ADDON_ATTACH_BOTH_CHAMBERS:
	{
		auto addon						= item->O.getModule<MAddon>();
		addon->startAttaching			(
			(tag == INVENTORY_ADDON_ATTACH_BOTH_CHAMBERS) ?
			nullptr :
			static_cast<CAddonSlot*>(m_UIPropertiesBox->GetClickedItem()->GetData())
		);
		break;
	}
	case INVENTORY_REPAIR:
		TryRepairItem					(this, 0);
		break;
	case INVENTORY_PLAY_ACTION:
		if (CPda* pPda = smart_cast<CPda*>(item))
			pPda->PlayScriptFunction	();
		break;
	}
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

void CUIActorMenu::pickup_item(PIItem item, eItemPlace type, u16 slot_id)
{
	u16 parent_id						= item->parent_id();
	if (parent_id != u16_max)
		CGameObject::sendEvent			(GE_TRADE_SELL, parent_id, item->object_id(), true);

	item->m_ItemCurrPlace.type			= type;
	item->m_ItemCurrPlace.slot_id		= slot_id;
	item->O.transfer					(m_pActorInvOwner->object_id(), true);
}

void CUIActorMenu::onInventoryAction(CInventoryItem CP$ item, const SInvItemPlace* prev)
{
	if (!IsShown() || !item->getIcon())
		return;

	if (auto old_owner = item->getIcon()->OwnerList())
		if (GetListType(old_owner) == iTrashSlot)
			m_pTrashList->setValid		(false);

	u16 parent_id						= item->parent_id();
	if (parent_id == m_pActorInvOwner->object_id())
	{
		if (prev)
			process_place				(*prev);
		process_place					(item->m_ItemCurrPlace);
	}
	else if (parent_id != u16_max)
	{
		if (m_currMenuMode == mmDeadBodySearch &&
			(m_pPartnerInvOwner && parent_id == m_pPartnerInvOwner->object_id() ||
				m_pInvBox && parent_id == m_pInvBox->ID() ||
				m_pContainer && parent_id == m_pContainer->O.ID()))
			m_pDeadBodyBagList->setValid(false);
		else if (m_pBag && parent_id == m_pBag->O.ID())
			GetListByType(iActorBag)->setValid(false);
	}
	else
		m_pTrashList->setValid			(false);
}

void CUIActorMenu::process_place(SInvItemPlace CR$ place)
{
	switch (place.type)
	{
	case eItemPlaceSlot:
	{
		auto slot						= GetSlotList(place.slot_id);
		slot->setValid					(false);
		break;
	}
	case eItemPlacePocket:
	{
		auto& pocket					= m_pInvPocket[place.slot_id];
		pocket->setValid				(false);
		break;
	}
	case eItemPlaceTrade:
		m_pTradeActorList->setValid		(false);
		break;
	}
}
