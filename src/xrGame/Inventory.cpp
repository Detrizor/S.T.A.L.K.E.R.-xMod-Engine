////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory.h"
#include "actor.h"
#include "CustomOutfit.h"
#include "trade.h"
#include "weapon.h"
#include "grenade.h"

#include "ui/UIInventoryUtilities.h"
#include "ui/UIActorMenu.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "uigamecustom.h"
#include "clsid_game.h"
#include "static_cast_checked.hpp"
#include "player_hud.h"
#include "CustomDetector.h"
#include "ActorBackpack.h"

using namespace InventoryUtilities;
//Alundaio
#include "../../xrServerEntities/script_engine.h" 
using namespace luabind; 
//-Alundaio

#define NO_ID u16(-1)

// what to block
u16	INV_STATE_BLOCK_ALL = 0xffff;
u16	INV_STATE_LADDER = INV_STATE_BLOCK_ALL;
u16	INV_STATE_CAR = INV_STATE_LADDER;
u16	INV_STATE_INV_WND = INV_STATE_BLOCK_ALL;
u16	INV_STATE_BUY_MENU = INV_STATE_BLOCK_ALL;

CInventorySlot::CInventorySlot() 
{
	m_pIItem				= NULL;
	m_bAct					= true;
	m_bPersistent			= false;
}

CInventorySlot::~CInventorySlot() 
{
}

bool CInventorySlot::CanBeActivated() const 
{
	return (m_bAct);
};

CInventory::CInventory() 
{
	m_iActiveSlot								= NO_ACTIVE_SLOT;
	m_iNextActiveSlot							= NO_ACTIVE_SLOT;
	m_iPrevActiveSlot							= NO_ACTIVE_SLOT;
	
	m_iReturnPlace								= 0;
	m_iReturnSlot								= 0;
	m_iNextActiveItemID							= NO_ID;
	m_iNextLeftItemID							= NO_ID;

	//Alundaio: Dynamically create as many slots as we may define in system.ltx
	string256	slot_persistent;
	string256	slot_active;
	xr_strcpy(slot_persistent,"slot_persistent_1");
	xr_strcpy(slot_active,"slot_active_1");
	
	u16 k=1;
	while( pSettings->line_exist("inventory", slot_persistent) && pSettings->line_exist("inventory", slot_active)){
		m_last_slot = k;
		
		m_slots.resize(k+1); //slot+1 because [0] is the inactive slot
		
		m_slots[k].m_bPersistent = !!pSettings->r_bool("inventory",slot_persistent);
		m_slots[k].m_bAct = !!pSettings->r_bool("inventory",slot_active);
		
		k++;

		xr_sprintf		(slot_persistent,"%s%d","slot_persistent_",k);
		xr_sprintf		(slot_active,"%s%d","slot_active_",k);
	}
	
	m_blocked_slots.resize(k+1);
	
	for (u16 i = 0; i <= k; ++i)
	{
		m_blocked_slots[i] = 0;
	}
	//-Alundaio

	m_pockets_count			= pSettings->r_u8("inventory", "max_pockets_count");
	m_pockets.resize		(m_pockets_count);

	m_bSlotsUseful								= true;

	m_fTotalWeight								= -1.f;
	m_dwModifyFrame								= 0;
	m_drop_last_frame							= false;
	
	InitPriorityGroupsForQSwitch				();
	m_next_item_iteration_time					= 0;

	m_change_after_deactivate = false;

	m_bBoltPickUp = false;
	
	m_bRuckAllowed			= !!pSettings->r_bool("inventory", "ruck_allowed");
	m_iToDropID				= 0;
	m_iRuckVboxID			= 0;
	m_fRuckVboxCapacity		= 0.f;
}


CInventory::~CInventory() 
{
}

void CInventory::Clear()
{
	m_all.clear							();
	m_ruck.clear						();
	
	for(u16 i=FirstSlot(); i<=LastSlot(); i++)
		m_slots[i].m_pIItem				= NULL;

	for (u8 i = 0; i < m_pockets_count; i++)
		m_pockets[i].clear();

	m_pOwner							= NULL;

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();
}

void CInventory::Take(CGameObject *pObj, bool bNotActivate, bool strict_placement)
{
	CInventoryItem* pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	VERIFY								(pIItem->m_pInventory==NULL);
	VERIFY								(CanTakeItem(pIItem));
	
	pIItem->m_pInventory				= this;
	pIItem->SetDropManual				(FALSE);
	pIItem->AllowTrade					();
	//if net_Import for pObj arrived then the pObj will pushed to CrPr list (correction prediction)
	//usually net_Import arrived for objects that not has a parent object..
	//for unknown reason net_Import arrived for object that has a parent, so correction prediction schema will crash
	Level().RemoveObject_From_4CrPr		(pObj);

	m_all.push_back						(pIItem);
	
	if (!strict_placement)
		pIItem->m_ItemCurrPlace.type			= eItemPlaceUndefined;
	if (pIItem->m_ItemCurrPlaceBackup.type)
	{
		pIItem->m_ItemCurrPlace					= pIItem->m_ItemCurrPlaceBackup;
		pIItem->m_ItemCurrPlaceBackup.type		= eItemPlaceUndefined;
	}
	
	switch(pIItem->m_ItemCurrPlace.type)
	{
	case eItemPlaceSlot:{
		u16 slot								= pIItem->m_ItemCurrPlace.slot_id;
		if (slot == LEFT_HAND_SLOT || slot == RIGHT_HAND_SLOT || slot == BOTH_HANDS_SLOT)
			ActivateItem						(pIItem);
		else if (!Slot(slot, pIItem, bNotActivate, strict_placement))
			pIItem->m_ItemCurrPlace.type		= eItemPlaceUndefined;
		}break;
	case eItemPlacePocket:
		if (!Pocket(pIItem, pIItem->m_ItemCurrPlace.slot_id, true))
			pIItem->m_ItemCurrPlace.type		= eItemPlaceUndefined;
		break;
	case eItemPlaceRuck:
		if (!Ruck(pIItem, strict_placement))
			pIItem->m_ItemCurrPlace.type = eItemPlaceUndefined;
		break;
	}

	if (pIItem->CurrPlace() == eItemPlaceUndefined)
	{
		if (pIItem->RuckDefault() || !m_pOwner->is_alive() || !Slot(pIItem->BaseSlot(), pIItem))
			Ruck(pIItem, strict_placement);
	}
	
	m_pOwner->OnItemTake				(pIItem);

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	pIItem->object().processing_deactivate();
	VERIFY								(pIItem->CurrPlace() != eItemPlaceUndefined);

	if (CurrentGameUI())
	{
		CObject* pActor_owner			= smart_cast<CObject*>(m_pOwner);
		if (Level().CurrentViewEntity() == pActor_owner)
			CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_TAKE);
		else if(CurrentGameUI()->GetActorMenu().GetMenuMode()==mmDeadBodySearch)
		{
			if (m_pOwner == CurrentGameUI()->GetActorMenu().GetPartner())
				CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_TAKE);
		}
	}

	if (pIItem->BaseSlot() == BOLT_SLOT)
	{
		CGrenade* pGrenade						= smart_cast<CGrenade*>(pIItem);
		CMissile* pMissile						= smart_cast<CMissile*>(pIItem);
		if ((pGrenade && !pGrenade->Useful()) || (pMissile && !pMissile->Useful()))
		{
			m_pOwner->GiveObject				(*pIItem->m_section_id);
			if (pIItem->CurrSlot() == pIItem->HandSlot())
				m_bBoltPickUp					= true;
			pIItem->object().DestroyObject		();
		}
		else if (m_bBoltPickUp)
		{
			ActivateItem						(pIItem);
			m_bBoltPickUp						= false;
		}
	}
}

bool CInventory::DropItem(CGameObject *pObj, bool just_before_destroy, bool dont_create_shell) 
{
	CInventoryItem *pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	VERIFY								(pIItem->m_pInventory);
	VERIFY								(pIItem->m_pInventory==this);
	VERIFY								(pIItem->m_ItemCurrPlace.type!=eItemPlaceUndefined);
	
	pIItem->object().processing_activate(); 
	u16 place		= pIItem->CurrPlace();
	switch (place)
	{
	case eItemPlaceRuck:{
			VERIFY(InRuck(pIItem));
			TIItemContainer::iterator temp_iter = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
			if (temp_iter != m_ruck.end())
			{
				m_ruck.erase(temp_iter);
			} else
			{
				Msg("! ERROR: CInventory::Drop item not found in ruck...");
			}
		}break;
	case eItemPlaceSlot:{
			VERIFY			(InSlot(pIItem));
			if (pIItem == ActiveItem() || pIItem == LeftItem())
			{
				if (smart_cast<CActor*>(m_pOwner))
					ActivateItem(pIItem);
				else
					Activate(NO_ACTIVE_SLOT, just_before_destroy);
			}

			m_slots[pIItem->CurrSlot()].m_pIItem = NULL;							
			pIItem->object().processing_deactivate();
		}break;
	case eItemPlacePocket:{
		TIItemContainer& pocket			= m_pockets[pIItem->CurrPocket()];
		TIItemContainer::iterator		it = std::find(pocket.begin(), pocket.end(), pIItem);
		if (it != pocket.end())
			pocket.erase(it);
		else
			Msg("! ERROR: CInventory::Drop item not found in pocket...");
		pIItem->object().processing_deactivate();
		}break;
	default:
		NODEFAULT;
	};
	TIItemContainer::iterator it = std::find(m_all.begin(), m_all.end(), pIItem);
	if(it!=m_all.end())
		m_all.erase(std::find(m_all.begin(), m_all.end(), pIItem));
	else
		Msg("! CInventory::Drop item not found in inventory!!!");

	pIItem->m_pInventory = NULL;


	m_pOwner->OnItemDrop	(smart_cast<CInventoryItem*>(pObj), just_before_destroy);

	CalcTotalWeight					();
	CalcTotalVolume					();
	InvalidateState					();
	m_drop_last_frame				= true;

	if( CurrentGameUI() )
	{
		CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);

		if (Level().CurrentViewEntity() == pActor_owner)
			CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_REJECT);
	};
	if (smart_cast<CWeapon*>(pObj))
	{
		Fvector dir = Actor()->Direction();
		dir.y = sin(-45.f * PI / 180.f);
		dir.normalize();
		smart_cast<CWeapon*>(pObj)->SetActivationSpeedOverride(dir.mul(7));
		pObj->H_SetParent(nullptr, dont_create_shell);
	}
	else
		pObj->H_SetParent(nullptr, dont_create_shell);
	return							true;
}

void CInventory::SendItemToDrop(PIItem item)
{
	NET_Packet						P;
	CGameObject::u_EventGen			(P, GE_OWNERSHIP_REJECT, m_pOwner->object_id());
	P.w_u16							(item->object_id());
	CGameObject::u_EventSend		(P);
}

//положить вещь в слот
bool CInventory::Slot(u16 slot_id, PIItem pIItem, bool bNotActivate, bool strict_placement) 
{
	VERIFY(pIItem);
	
	if (slot_id == NO_ACTIVE_SLOT || ItemFromSlot(slot_id) == pIItem)
		return false;

	if (!CanPutInSlot(pIItem, slot_id))
		return false;

	m_slots[slot_id].m_pIItem = pIItem;
	
	//удалить из инвентаря
	TIItemContainer::iterator it_ruck = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
	if (it_ruck != m_ruck.end())
		m_ruck.erase(it_ruck);

	bool in_slot = InSlot(pIItem);
	if (in_slot && (pIItem->CurrSlot() != slot_id))
	{
		if (GetActiveSlot() == pIItem->CurrSlot())
			Activate(NO_ACTIVE_SLOT);
		m_slots[pIItem->CurrSlot()].m_pIItem = NULL;
	}

	if (!in_slot && pIItem->CurrPlace() == eItemPlacePocket)
	{
		TIItemContainer& pocket				= m_pockets[pIItem->CurrPocket()];
		TIItemContainer::iterator it		= std::find(pocket.begin(), pocket.end(), pIItem);
		if (it != pocket.end())
			pocket.erase(it);
	}

	CUIActorMenu& actor_menu = CurrentGameUI()->GetActorMenu();
	if (slot_id == RIGHT_HAND_SLOT || slot_id == BOTH_HANDS_SLOT)
	{
		CHudItem* hi = pIItem->cast_hud_item();
		if (hi)
			hi->ActivateItem();
		else
			actor_menu.PlaySnd(eItemToHands);
		if (!m_bRuckAllowed && smart_cast<CBackpack*>(pIItem))
			actor_menu.ToggleRuckContainer(pIItem);
	}
	else if (slot_id == LEFT_HAND_SLOT)
	{
		CCustomDetector* det = smart_cast<CCustomDetector*>(pIItem);
		if (det)
			det->ToggleDetector(g_player_hud->attached_item(0) != NULL);
		else
			actor_menu.PlaySnd(eItemToHands);
	}
	else if (smart_cast<CActor*>(m_pOwner))
		actor_menu.PlaySnd(eItemToSlot);

	SInvItemPlace p					= pIItem->m_ItemCurrPlace;
	m_pOwner->OnItemSlot			(pIItem, pIItem->m_ItemCurrPlace);
	pIItem->m_ItemCurrPlace.type	= eItemPlaceSlot;
	pIItem->m_ItemCurrPlace.slot_id = slot_id;
	pIItem->OnMoveToSlot			(p);
	
	pIItem->object().processing_activate();

	return						true;
}

bool CInventory::Ruck(PIItem pIItem, bool strict_placement) 
{
	if(!strict_placement && !CanPutInRuck(pIItem)) return false;

	bool in_slot = InSlot(pIItem);
	if (in_slot) 
	{
		if(GetActiveSlot() == pIItem->CurrSlot()) 
			Activate(NO_ACTIVE_SLOT);

		m_slots[pIItem->CurrSlot()].m_pIItem = NULL;
	}

	if (!in_slot && pIItem->CurrPlace() == eItemPlacePocket)
	{
		TIItemContainer& pocket				= m_pockets[pIItem->CurrPocket()];
		TIItemContainer::iterator it		= std::find(pocket.begin(), pocket.end(), pIItem);
		if (it != pocket.end())
			pocket.erase(it);
	}
	
	m_ruck.insert									(m_ruck.end(), pIItem); 
	
	CalcTotalWeight									();
	CalcTotalVolume									();
	InvalidateState									();

	m_pOwner->OnItemRuck							(pIItem, pIItem->m_ItemCurrPlace);
	SInvItemPlace prev_place						= pIItem->m_ItemCurrPlace;
	pIItem->m_ItemCurrPlace.type					= eItemPlaceRuck;
	pIItem->OnMoveToRuck							(prev_place);

	if (in_slot)
		pIItem->object().processing_deactivate		();

	return true;
}

bool CInventory::Pocket(PIItem pIItem, u16 pocket_id, bool forced) 
{
	if (!forced && !CanPutInPocket(pIItem, pocket_id))
		return false;

	bool in_slot									= InSlot(pIItem);
	if (in_slot) 
		m_slots[pIItem->CurrSlot()].m_pIItem		= NULL;
	else if (pIItem->CurrPlace() == eItemPlacePocket)
	{
		TIItemContainer& pocket				= m_pockets[pIItem->CurrPocket()];
		TIItemContainer::iterator it		= std::find(pocket.begin(), pocket.end(), pIItem);
		if (it != pocket.end())
			pocket.erase(it);
	}
	else
	{
		TIItemContainer::iterator it				= std::find(m_ruck.begin(), m_ruck.end(), pIItem); 
		if (m_ruck.end() != it) 
			m_ruck.erase							(it);
	}
	
	m_pockets[pocket_id].push_back(pIItem);

	CalcTotalWeight					();
	CalcTotalVolume					();
	InvalidateState					();

	SInvItemPlace p						= pIItem->m_ItemCurrPlace;
	pIItem->m_ItemCurrPlace.type		= eItemPlacePocket;
	pIItem->m_ItemCurrPlace.slot_id		= pocket_id;
	m_pOwner->OnItemRuck				(pIItem, p);
	pIItem->OnMoveToRuck				(p);

	if (in_slot)
		pIItem->object().processing_deactivate		();
	pIItem->object().processing_activate			();

	if (smart_cast<CActor*>(m_pOwner))
		CurrentGameUI()->GetActorMenu().PlaySnd(eItemToRuck);

	return true;
}

PIItem CInventory::ItemFromSlot(u16 slot) const
{
	return (slot == NO_ACTIVE_SLOT) ? NULL : m_slots[slot].m_pIItem;
}

void CInventory::SendActionEvent(u16 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor) return;

	NET_Packet		P;
	pActor->u_EventGen		(P,GE_INV_ACTION, pActor->ID());
	P.w_u16					(cmd);
	P.w_u32					(flags);
	P.w_s32					(pActor->GetZoomRndSeed());
	P.w_s32					(pActor->GetShotRndSeed());
	pActor->u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
};

bool CInventory::Action(u16 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	
	if (pActor)
	{
		switch(cmd)
		{
			case kWPN_FIRE:
			{
				pActor->SetShotRndSeed();
			}break;
			case kWPN_ZOOM : 
			{
				pActor->SetZoomRndSeed();
			}break;
		};
	};

	if (g_pGameLevel && OnClient() && pActor) 
	{
		switch(cmd)
		{
		case kUSE:		break;
		
		case kDROP:		
			{
				SendActionEvent	(cmd, flags);
				return			true;
			}break;

		case kWPN_NEXT:
		case kWPN_RELOAD:
		case kWPN_FIRE:
		case kWPN_FUNC:
		case kWPN_FIREMODE_NEXT:
		case kWPN_FIREMODE_PREV:
		case kWPN_ZOOM	 : 
		case kTORCH:
		case kNIGHT_VISION:
			{
				SendActionEvent(cmd, flags);
			}break;
		}
	}


	if (ActiveItem() && ActiveItem()->Action(cmd, flags) || LeftItem() && LeftItem()->Action(cmd, flags))
		return true;

	bool b_send_event = false;
	switch(cmd) 
	{
	case kWPN_1:
	case kWPN_2:
	case kWPN_3:
	case kWPN_4:
	case kWPN_5:
	case kWPN_6:
	case kWPN_7:
		if (flags & CMD_START)
		{
			u16 slot						= u16(cmd - kWPN_1 + KNIFE_SLOT);
			PIItem slot_item				= ItemFromSlot(slot);
			if (slot_item)
			{
				u16 hand_slot				= slot_item->HandSlot();
				if (CanPutInSlot(slot_item, hand_slot))
					ActivateItem			(slot_item);
				else
				{
					PIItem hand_item		= ItemFromSlot(hand_slot);
					if (hand_item)
					{
						u16 hitem_slot		= hand_item->BaseSlot();
						if (hitem_slot == slot || ((hitem_slot == PRIMARY_SLOT || hitem_slot == SECONDARY_SLOT) && (slot == PRIMARY_SLOT || slot == SECONDARY_SLOT)))
							ActivateItem	(slot_item, eItemPlaceSlot, slot);
					}
				}
			}
			else
			{
				PIItem active_item			= ActiveItem();
				if (active_item)
				{
					u16 active_item_slot	= active_item->BaseSlot();
					if (active_item_slot == slot || ((active_item_slot == PRIMARY_SLOT || active_item_slot == SECONDARY_SLOT) && (slot == PRIMARY_SLOT || slot == SECONDARY_SLOT)))
						ActivateItem		(active_item, eItemPlaceSlot, slot);
				}
			}
		}
		break;
	}

	if (b_send_event && g_pGameLevel && OnClient() && pActor)
		SendActionEvent(cmd, flags);

	return false;
}

void CInventory::Activate(u16 slot, bool bForce)
{
	if (!OnServer())
		return;
	R_ASSERT2(slot <= LastSlot(), "wrong slot number");

	if (!smart_cast<CActor*>(m_pOwner))
	{
		PIItem tmp_item = NULL;
		if (slot != NO_ACTIVE_SLOT)
			tmp_item = ItemFromSlot(slot);

		if (tmp_item && IsSlotBlocked(tmp_item) && (!bForce))
		{
			//to restore after unblocking ...
			SetPrevActiveSlot(slot);
			return;
		}

		if (GetActiveSlot() == slot || (GetNextActiveSlot() == slot && !bForce))
		{
			m_iNextActiveSlot = slot;
			return;
		}

		if (slot != NO_ACTIVE_SLOT && !m_slots[slot].CanBeActivated())
			return;

		if (GetActiveSlot() == NO_ACTIVE_SLOT)	//активный слот не выбран
		{
			if (tmp_item)
				m_iNextActiveSlot = slot;
			else
			{
				if (slot == GRENADE_SLOT)	//fake for grenade
				{
					PIItem gr = SameSlot(GRENADE_SLOT, NULL);
					if (gr)
						Slot(GRENADE_SLOT, gr);
				}
			}
		}
		else if (slot == NO_ACTIVE_SLOT || tmp_item)	//активный слот задействован
		{
			PIItem active_item = ActiveItem();
			if (active_item && !bForce)
			{
				CHudItem* tempItem = active_item->cast_hud_item();
				R_ASSERT2(tempItem, active_item->object().cNameSect().c_str());
				tempItem->SendDeactivateItem();
			}
			else //in case where weapon is going to destroy
			{
				if (tmp_item)
					tmp_item->ActivateItem();
				m_iActiveSlot = slot;
			}
			m_iNextActiveSlot = slot;
		}
	}
}

void CInventory::ActivateItem(PIItem item, u16 return_place, u16 return_slot)
{
	if (!item)
	{
		item = ActiveItem();
		if (!item)
			return;
	}

	PIItem left_item = LeftItem();
	PIItem active_item = ActiveItem();
	u16 hand = item->HandSlot();

	if (left_item && hand != RIGHT_HAND_SLOT)
	{
		CCustomDetector* det = smart_cast<CCustomDetector*>(left_item);
		if (det)
		{
			if (det->IsHiding())
				m_iNextLeftItemID = left_item->object_id();
			else
			{
				det->HideDetector(true);
				m_iNextLeftItemID = NO_ID;
			}
		}
		else
			m_iNextLeftItemID = NO_ID;
	}

	if (active_item && (hand != LEFT_HAND_SLOT || active_item->HandSlot() == BOTH_HANDS_SLOT))
	{
		CHudItem* hi = active_item->cast_hud_item();
		if (hi)
		{
			if (hi->IsHiding())
				m_iNextActiveItemID = active_item->object_id();
			else
			{
				if (!hi->IsHidden())
					hi->SendDeactivateItem();
				m_iNextActiveItemID = NO_ID;
			}
		}
		else
			m_iNextActiveItemID = NO_ID;
	}

	if (item == left_item || item == active_item)
	{
		if (return_place)
		{
			m_iReturnPlace			= return_place;
			m_iReturnSlot			= return_slot;
		}
	}
	else
	{
		if (hand == LEFT_HAND_SLOT)
			m_iNextLeftItemID		= item->object_id();
		else
			m_iNextActiveItemID		= item->object_id();
	}
}

void CInventory::Update()
{
	if (OnServer())
	{
		CActor* pActor = smart_cast<CActor*>(m_pOwner);
		if (pActor)
		{
			PIItem active_item = ActiveItem();
			u16 active_item_id = (active_item) ? active_item->object_id() : NO_ID;
			bool active_swap = (active_item_id != m_iNextActiveItemID);
			PIItem left_item = LeftItem();
			u16 left_item_id = (left_item) ? left_item->object_id() : NO_ID;
			bool left_swap = (left_item_id != m_iNextLeftItemID);
			if (active_swap || left_swap)
			{
				bool next_active_id = (m_iNextActiveItemID != NO_ID);
				PIItem next_active_item = (next_active_id) ? get_object_by_id(m_iNextActiveItemID) : NULL;
				if (next_active_id && !next_active_item)
					m_iNextActiveItemID = NO_ID;
				bool next_left_id = (m_iNextLeftItemID != NO_ID);
				PIItem next_left_item = (next_left_id) ? get_object_by_id(m_iNextLeftItemID) : NULL;
				if (next_left_id && !next_left_item)
					m_iNextLeftItemID = NO_ID;

				bool block = false;

				if (active_item && (active_swap || (next_left_item && active_item->HandSlot() != RIGHT_HAND_SLOT)))
				{
					CHudItem* hi = active_item->cast_hud_item();
					if (!hi || hi->IsHidden())
					{
						if (m_iReturnPlace)
						{
							if (m_iReturnPlace == eItemPlacePocket)
								ToPocket(active_item, m_iReturnSlot);
							else if (m_iReturnPlace == eItemPlaceRuck || !ToSlot(m_iReturnSlot, active_item))
								ToRuck(active_item);
							m_iReturnPlace		= 0;
							m_iReturnSlot		= 0;
						}
						else
						{
							u16 active_item_slot = active_item->BaseSlot();
							if (!ToSlot(active_item_slot, active_item))
							{
								if (!(active_item_slot == PRIMARY_SLOT && ToSlot(SECONDARY_SLOT, active_item)) && !(active_item_slot == SECONDARY_SLOT && ToSlot(PRIMARY_SLOT, active_item)))
								{
									if (next_active_item)
									{
										u16 slot = next_active_item->CurrSlot();
										if (slot == active_item_slot || ((slot == PRIMARY_SLOT || slot == SECONDARY_SLOT) && (active_item_slot == PRIMARY_SLOT || active_item_slot == SECONDARY_SLOT)))
										{
											ToRuck(next_active_item);
											ToSlot(slot, active_item);
										}
										else
											ToRuck(active_item);
									}
									else
										ToRuck(active_item);
								}
							}
						}
					}
					else
						block = true;
				}

				if (left_item && (left_swap || (next_active_item && next_active_item->HandSlot() != RIGHT_HAND_SLOT)))
				{
					CCustomDetector* det = smart_cast<CCustomDetector*>(left_item);
					if (!det || det->IsHidden())
					{
						if (m_iReturnPlace)
						{
							if (m_iReturnPlace == (u16(-1)))
							{
								det->ActivateItem();
								m_iNextLeftItemID = left_item_id;
							}
							else if (m_iReturnPlace == eItemPlacePocket)
								ToPocket(left_item, m_iReturnSlot);
							else if (m_iReturnSlot == eItemPlaceRuck || !ToSlot(m_iReturnSlot, left_item))
								ToRuck(left_item);
							m_iReturnPlace		= 0;
							m_iReturnSlot		= 0;
						}
						else
							ToRuck(left_item);
					}
					else
						block = true;
				}

				if (block)
					return;

				if (active_swap && next_active_item)
					ToSlot(next_active_item->HandSlot(), next_active_item);

				if (left_swap && next_left_item)
					ToSlot(LEFT_HAND_SLOT, next_left_item);

				CalcTotalVolume();
			}
		}
		else
		{
			if (m_iActiveSlot != m_iNextActiveSlot)
			{
				CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);
				if (Level().CurrentViewEntity() == pActor_owner)
				{
					if ((m_iNextActiveSlot != NO_ACTIVE_SLOT) &&
						ItemFromSlot(m_iNextActiveSlot) &&
						!g_player_hud->allow_activation(ItemFromSlot(m_iNextActiveSlot)->cast_hud_item())
						)
						return;
				}
				if (ActiveItem())
				{
					CHudItem* hi = ActiveItem()->cast_hud_item();

					if (!hi->IsHidden())
					{
						if (hi->GetState() == CHUDState::eIdle && hi->GetNextState() == CHUDState::eIdle)
							hi->SendDeactivateItem();

						UpdateDropTasks();
						return;
					}
				}

				if (m_change_after_deactivate)
					ActivateNextGrenage();

				if (GetNextActiveSlot() != NO_ACTIVE_SLOT)
				{
					PIItem tmp_next_active = ItemFromSlot(GetNextActiveSlot());
					if (tmp_next_active)
					{
						if (IsSlotBlocked(tmp_next_active))
						{
							Activate(m_iActiveSlot);
							return;
						}
						else
						{
							tmp_next_active->ActivateItem();
						}
					}
				}

				m_iActiveSlot = GetNextActiveSlot();
			}
			else if ((GetNextActiveSlot() != NO_ACTIVE_SLOT) && ActiveItem() && ActiveItem()->cast_hud_item()->IsHidden())
				ActiveItem()->ActivateItem();
		}
	}
	UpdateDropTasks();
}

bool CInventory::ToSlot(u16 slot, PIItem item)
{
	CUIActorMenu& actor_menu		= CurrentGameUI()->GetActorMenu();
	return							(actor_menu.GetMenuMode() != mmUndefined) ? actor_menu.ToSlot(slot, item) : Slot(slot, item);
}

bool CInventory::ToRuck(PIItem item)
{
	CUIActorMenu& actor_menu		= CurrentGameUI()->GetActorMenu();
	return							(actor_menu.GetMenuMode() != mmUndefined) ? actor_menu.ToBag(item) : Ruck(item);
}

bool CInventory::ToPocket(PIItem item, u16 pocket_id)
{
	CUIActorMenu& actor_menu		= CurrentGameUI()->GetActorMenu();
	return							(actor_menu.GetMenuMode() != mmUndefined) ? actor_menu.ToPocket(item, pocket_id) : Pocket(item, pocket_id);
}

PIItem CInventory::ActiveItem() const
{
	if (smart_cast<CActor*>(m_pOwner))
	{
		PIItem both_hands_item = ItemFromSlot(BOTH_HANDS_SLOT);
		PIItem right_hand_item = ItemFromSlot(RIGHT_HAND_SLOT);
		R_ASSERT2(!(both_hands_item && right_hand_item), "hand slots' items superposition");
		return (both_hands_item) ? both_hands_item : right_hand_item;
	}
	return (m_iActiveSlot == NO_ACTIVE_SLOT) ? NULL : ItemFromSlot(m_iActiveSlot);
}

u16 CInventory::GetActiveSlot() const
{
	if (smart_cast<CActor*>(m_pOwner))
	{
		PIItem active_item = ActiveItem();
		return (active_item) ? active_item->BaseSlot() : NO_ACTIVE_SLOT;
	}
	return m_iActiveSlot;
}

void move_item_from_to(u16 from_id, u16 to_id, u16 what_id);
void CInventory::UpdateDropTasks()
{
	//проверить слоты
	for(u16 i=FirstSlot(); i<=LastSlot(); ++i)	
	{
		PIItem itm				= ItemFromSlot(i);
		if(itm)
			UpdateDropItem		(itm);
	}
	
	bool any_proper				= false;
	bool check					= !m_bRuckAllowed && smart_cast<CActor*>(m_pOwner) && (m_iRuckVboxID >= 0);
	bool check2					= (m_iRuckVboxID > 0) && ((CurrentGameUI()->GetActorMenu().GetMenuMode() == mmUndefined) || (ItemFromSlot(BOTH_HANDS_SLOT) != m_pContainer));
	for (TIItemContainer::iterator it = m_ruck.begin(), it_e = m_ruck.end(); it != it_e; ++it)
	{
		PIItem item				= *it;
		if (!item->Weight())
			continue;
		any_proper				= true;

		UpdateDropItem			(item);
		
		if (check2)
			move_item_from_to	(m_pOwner->object_id(), (u16)m_iRuckVboxID, (*it)->object_id());
		else if (check && !ProcessItem(item))
			SendItemToDrop		(item);

		if (item->object_id() == m_iToDropID)
		{
			SendItemToDrop		(item);
			m_iToDropID			= 0;
		}
	}
	
	if (check2)
	{
		m_ruck_vbox.clear	();
		m_iRuckVboxID		= -1;
	}

	if (m_iRuckVboxID < 0 && !any_proper)
	{
		m_iRuckVboxID											= 0;
		m_fRuckVboxCapacity										= 0.f;
		CurrentGameUI()->GetActorMenu().ToggleRuckContainer		(NULL);
	}

	if (m_drop_last_frame)
	{
		m_drop_last_frame			= false;
		m_pOwner->OnItemDropUpdate	();
	}
}

bool CInventory::ProcessItem(PIItem item)
{
	if (m_iRuckVboxID)
	{
		if (!m_fRuckVboxCapacity)
			m_fRuckVboxCapacity		= smart_cast<CInventoryBox*>(Level().Objects.net_Find((u16)m_iRuckVboxID))->GetCapacity();

		float volume				= 0.f;
		for (TIItemContainer::const_iterator it = m_ruck_vbox.begin(), it_e = m_ruck_vbox.end(); it != it_e; ++it)
		{
			PIItem iitem			= *it;
			if (iitem == item)
				return				true;
			if (iitem && iitem->parent_id() == m_pOwner->object_id())
				volume				+= iitem->Volume();
		}
		if (volume < m_fRuckVboxCapacity && (volume + item->Volume()) < (m_fRuckVboxCapacity + 0.1f))
		{
			m_ruck_vbox.push_back	(item);
			return					true;
		}
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		if (item->m_section_id == ACTOR_DEFS::g_quick_use_slots[i] && ToPocket(item, i))
			return true;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		if (ToPocket(item, i))
			return true;
	}

	if (!item->RuckDefault() && ToSlot(item->BaseSlot(), item))
		return true;

	if (CanPutInSlot(item, item->HandSlot()))
	{
		ActivateItem		(item);
		return				true;
	}

	return false;
}

void CInventory::UpdateDropItem(PIItem pIItem)
{
	if( pIItem->GetDropManual() )
	{
		pIItem->SetDropManual(FALSE);
		pIItem->DenyTrade();

		if ( OnServer() ) 
		{
			NET_Packet					P;
			pIItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
			P.w_u16						(u16(pIItem->object().ID()));
			pIItem->object().u_EventSend(P);
		}
	}// dropManual
}

//ищем гранату такоже типа
PIItem CInventory::Same(const PIItem pIItem) const
{
	for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
	{
		const PIItem l_pIItem		= *it;
		if ((l_pIItem != pIItem) && !xr_strcmp(l_pIItem->object().cNameSect(), pIItem->object().cNameSect()))
			return					l_pIItem;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
		{
			const PIItem item		= *it;
			if (item != pIItem && item->m_section_id == pIItem->m_section_id)
				return				item;
		}
	}

	return NULL;
}

//ищем вещь для слота 
PIItem CInventory::SameSlot(const u16 slot, PIItem pIItem) const
{
	if (slot == NO_ACTIVE_SLOT)
		return NULL;

	for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
	{
		PIItem _pIItem		= *it;
		if (_pIItem != pIItem && _pIItem->BaseSlot() == slot)
			return			_pIItem;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
		{
			const PIItem item		= *it;
			if (item != pIItem && item->BaseSlot() == slot)
				return				item;
		}
	}

	return NULL;
}

//найти в инвенторе вещь с указанным именем
PIItem CInventory::Get(LPCSTR name, int pocket_id) const
{
	if (pocket_id == -1)
		for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
		{
			PIItem pIItem		= *it;
			if (!xr_strcmp(pIItem->object().cNameSect(), name)) 
				return			pIItem;
		}

	u8 a		= (pocket_id == -1) ? 0 : (u8)pocket_id;
	u8 b		= (pocket_id == -1) ? m_pockets_count : a + 1;
	for (u8 i = a; i < b; i++)
	{
		for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
		{
			const PIItem item		= *it;
			if (item->m_section_id == name)
				return				item;
		}
	}

	return NULL;
}

PIItem CInventory::Get(CLASS_ID cls_id) const
{
	for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
	{
		PIItem pIItem		= *it;
		if (pIItem->object().CLS_ID == cls_id) 
			return			pIItem;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
		{
			PIItem item		= *it;
			if (item->object().CLS_ID == cls_id)
				return		item;
		}
	}

	return NULL;
}

PIItem CInventory::Get(const u16 id) const
{
	for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
	{
		PIItem pIItem		= *it;
		if (pIItem->object().ID() == id) 
			return			pIItem;
	}

	for (u8 i = 0; i < m_pockets_count; i++)
	{
		for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
		{
			PIItem item		= *it;
			if (item->object_id() == id)
				return		item;
		}
	}

	return NULL;
}

//search both (ruck and belt)
PIItem CInventory::GetAny(LPCSTR name) const
{
	return Get(name);
}

PIItem CInventory::item(CLASS_ID cls_id) const
{
	const TIItemContainer &list = m_all;

	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if (pIItem->object().CLS_ID == cls_id) 
			return pIItem;
	}
	return NULL;
}

float CInventory::TotalWeight() const
{
	VERIFY(m_fTotalWeight>=0.f);
	return m_fTotalWeight;
}

float CInventory::TotalVolume() const
{
	VERIFY(m_fTotalVolume >= 0.f);
	return m_fTotalVolume;
}

float CInventory::CalcTotalWeight()
{
	float weight	= 0;
	for (TIItemContainer::const_iterator it = m_ruck.begin(), it_e = m_ruck.end(); it != it_e; ++it)
		weight		+= (*it)->Weight();
	m_fTotalWeight	= weight;
	return			weight;
}

float CInventory::CalcTotalVolume()
{
	float volume	= 0;
	for (TIItemContainer::const_iterator it = m_ruck.begin(), it_e = m_ruck.end(); it != it_e; ++it)
		volume		+= (*it)->Volume();
	m_fTotalVolume	= volume;
	return			volume;
}

u32 CInventory::dwfGetSameItemCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (!xr_strcmp(l_pIItem->object().cNameSect(), caSection))
            ++l_dwCount;
	}
	
	return		(l_dwCount);
}
u32		CInventory::dwfGetGrenadeCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().CLS_ID == CLSID_GRENADE_F1 || l_pIItem->object().CLS_ID == CLSID_GRENADE_RGD5)
			++l_dwCount;
	}

	return		(l_dwCount);
}

bool CInventory::bfCheckForObject(ALife::_OBJECT_ID tObjectID)
{
	TIItemContainer	&l_list = m_all;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().ID() == tObjectID)
			return(true);
	}
	return		(false);
}

CInventoryItem *CInventory::get_object_by_id(ALife::_OBJECT_ID tObjectID)
{
	TIItemContainer	&l_list = m_all;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().ID() == tObjectID)
			return	(l_pIItem);
	}
	return		(0);
}

//скушать предмет 
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
bool CInventory::Eat(PIItem pIItem)
{
	//устанаовить съедобна ли вещь
	CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
	if (!pItemToEat)
		return					false;

	if (pItemToEat->Empty())
		return					false;

	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(m_pOwner);
	if (!entity_alive)
		return					false;

	CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
	if (!IO)
		return					false;

	CInventory* pInventory = pItemToEat->m_pInventory;
	if (!pInventory || pInventory != this)
		return					false;
	if (pInventory != IO->m_inventory)
		return					false;
	if (pItemToEat->object().H_Parent()->ID() != entity_alive->ID())
		return					false;

	if (READ_IF_EXISTS(pSettings, r_bool, pIItem->m_section_id, "pda_trigger", false))
		return					(pIItem->GetCondition() > 0.f) ? CurrentGameUI()->ShowPdaMenu() : false;

	if (!pItemToEat->UseBy(entity_alive))
		return					false;

	luabind::functor<bool>	funct;
	if (ai().script_engine().functor("_G.CInventory__eat", funct))
	{
		if (!funct(smart_cast<CGameObject*>(pItemToEat->object().H_Parent())->lua_game_object(), (smart_cast<CGameObject*>(pIItem))->lua_game_object()))
			return false;
	}
	
	if (Actor()->m_inventory == this)
	{
		Actor()->callback(GameObject::eUseObject)((smart_cast<CGameObject*>(pIItem))->lua_game_object());
		CurrentGameUI()->GetActorMenu().SetCurrentItem(NULL);
	}

	if (pItemToEat->DrainOnUse())
		pItemToEat->ChangeRemainingUses(-1);

	if (pItemToEat->Empty() && pItemToEat->CanDelete())
		pIItem->SetDropManual(TRUE);

	return true;
}

bool CInventory::ClientEat(PIItem pIItem)
{
	CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
	if ( !pItemToEat )			return false;

	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(m_pOwner);
	if ( !entity_alive )		return false;

	CInventoryOwner* IO	= smart_cast<CInventoryOwner*>(entity_alive);
	if ( !IO )					return false;
	
	CInventory* pInventory = pItemToEat->m_pInventory;
	if ( !pInventory || pInventory != this )	return false;
	if ( pInventory != IO->m_inventory )		return false;
	if ( pItemToEat->object().H_Parent()->ID() != entity_alive->ID() )		return false;
	
	NET_Packet						P;
	CGameObject::u_EventGen			(P, GEG_PLAYER_ITEM_EAT, pIItem->parent_id());
	P.w_u16							(pIItem->object().ID());
	CGameObject::u_EventSend		(P);
	return true;
}

bool CInventory::InSlot(const CInventoryItem* pIItem) const
{
	if(pIItem->CurrPlace() != eItemPlaceSlot)	return false;

	VERIFY(m_slots[pIItem->CurrSlot()].m_pIItem == pIItem);

	return true;
}

bool CInventory::InRuck(const CInventoryItem* pIItem) const
{
	u16 id				= pIItem->object_id();
	for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
	{
		if ((*it)->object_id() == id)
			return		true;
	}
	return				false;
}

bool CInventory::CanPutInSlot(PIItem pIItem, u16 slot_id) const
{
	if (!m_bSlotsUseful) return false;

	if (!GetOwner()->CanPutInSlot(pIItem, slot_id)) return false;


	if (slot_id == HELMET_SLOT)
	{
		CCustomOutfit* pOutfit = m_pOwner->GetOutfit();
		if (pOutfit && !pOutfit->bIsHelmetAvaliable)
			return false;
	}

	if ((slot_id == LEFT_HAND_SLOT || slot_id == RIGHT_HAND_SLOT) && ItemFromSlot(BOTH_HANDS_SLOT))
		return false;
	if (slot_id == BOTH_HANDS_SLOT && (ItemFromSlot(LEFT_HAND_SLOT) || ItemFromSlot(RIGHT_HAND_SLOT)))
		return false;

	if (slot_id != NO_ACTIVE_SLOT && !ItemFromSlot(slot_id))
		return true;
	
	return false;
}

//проверяет можем ли поместить вещь в рюкзак,
//при этом реально ничего не меняется
bool CInventory::CanPutInRuck(PIItem pIItem) const
{
	return (!InRuck(pIItem));
}

bool CInventory::CanPutInPocket(PIItem pIItem, u16 pocket_id) const
{
	if (!PocketPresent(pocket_id) || !smart_cast<CActor*>(m_pOwner))
		return false;

	const TIItemContainer& pocket		= m_pockets[pocket_id];
	float volume						= 0.f;
	for (TIItemContainer::const_iterator it = pocket.begin(); pocket.end() != it; ++it)
		volume							+= (*it)->Volume();
	float max_volume					= m_pOwner->GetOutfit()->m_pockets[pocket_id];

	return (volume < max_volume && volume + pIItem->Volume() < max_volume + 0.1f);
}

bool CInventory::PocketPresent(u16 pocket_id) const
{
	CCustomOutfit* outfit		= m_pOwner->GetOutfit();
	return						(outfit && pocket_id < outfit->m_pockets.size());
}

void CInventory::EmptyPockets()
{
	for (u8 i = 0; i < m_pockets_count; i++)
		while (m_pockets[i].size())
			ToRuck(m_pockets[i].back());
}

u32	CInventory::dwfGetObjectCount()
{
	return		(m_all.size());
}

CInventoryItem	*CInventory::tpfGetObjectByIndex(int iIndex)
{
	if ((iIndex >= 0) && (iIndex < (int)m_all.size())) {
		TIItemContainer	&l_list = m_all;
		int			i = 0;
		for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it, ++i) 
			if (i == iIndex)
                return	(*l_it);
	}
	else {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"invalid inventory index!");
		return	(0);
	}
	R_ASSERT	(false);
	return		(0);
}

CInventoryItem	*CInventory::GetItemFromInventory(LPCSTR caItemName)
{
	TIItemContainer	&l_list = m_all;

	u32 crc = crc32(caItemName, xr_strlen(caItemName));

	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it)
		if ((*l_it)->object().cNameSect()._get()->dwCRC == crc){
			VERIFY(	0 == xr_strcmp( (*l_it)->object().cNameSect().c_str(), caItemName)  );
			return	(*l_it);
		}
	return	(0);
}


bool CInventory::CanTakeItem(CInventoryItem *inventory_item) const
{
	VERIFY			(inventory_item);
	VERIFY			(m_pOwner);

	if (inventory_item->object().getDestroy()) return false;

	if(!inventory_item->CanTake()) return false;

	for(TIItemContainer::const_iterator it = m_all.begin(); it != m_all.end(); it++)
		if((*it)->object().ID() == inventory_item->object().ID()) break;
	VERIFY3(it == m_all.end(), "item already exists in inventory",*inventory_item->object().cName());

	return	true;
}

void  CInventory::AddAvailableItems(TIItemContainer& items_container, bool for_trade, bool bOverride) const
{
	for(TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if (!for_trade || pIItem->CanTrade())
		{
			if (bOverride)
			{
				luabind::functor<bool> funct;
				if (ai().script_engine().functor("actor_menu_inventory.CInventory_ItemAvailableToTrade", funct))
				{
					if (!funct(m_pOwner->cast_game_object()->lua_game_object(),pIItem->cast_game_object()->lua_game_object()))
						continue;
				}
			}
			items_container.push_back(pIItem);
		}
	}

	if (m_bSlotsUseful)
	{
		u16 I = FirstSlot();
		u16 E = LastSlot();
		for (; I <= E; ++I)
		{
			PIItem item = ItemFromSlot(I);
			if (item && (!for_trade || item->CanTrade()))
			{
				if (!SlotIsPersistent(I) || item->BaseSlot() == GRENADE_SLOT)
				{
					if (bOverride)
					{
						luabind::functor<bool> funct;
						if (ai().script_engine().functor("actor_menu_inventory.CInventory_ItemAvailableToTrade", funct))
						{
							if (!funct(m_pOwner->cast_game_object()->lua_game_object(), item->cast_game_object()->lua_game_object()))
								continue;
						}
					}
					items_container.push_back(item);
				}
			}
		}
	}
}

bool CInventory::isBeautifulForActiveSlot	(CInventoryItem *pIItem)
{
	u16 I = FirstSlot();
	u16 E = LastSlot();
	for(;I<=E;++I)
	{
		PIItem item = ItemFromSlot(I);
		if (item && item->IsNecessaryItem(pIItem))
			return		(true);
	}
	return				(false);
}

//.#include "WeaponHUD.h"
void CInventory::Items_SetCurrentEntityHud(bool current_entity)
{
	TIItemContainer::iterator it;
	for(it = m_all.begin(); m_all.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		CWeapon* pWeapon = smart_cast<CWeapon*>(pIItem);
		if (pWeapon)
		{
			pWeapon->InitAddons();
			pWeapon->UpdateAddonsVisibility();
		}
	}
};

//call this only via Actor()->SetWeaponHideState()
void CInventory::SetSlotsBlocked(u16 mask, bool bBlock)
{
	R_ASSERT(OnServer() || Level().IsDemoPlayStarted());

	for(u16 i = FirstSlot(), ie = LastSlot(); i <= ie; ++i)
	{
		if(mask & (1<<i))
		{
			if (bBlock)
				BlockSlot(i);
			else
				UnblockSlot(i);
		}
	}
	
	if (bBlock)
	{
		TryDeactivateActiveSlot();	
	} else
	{
		TryActivatePrevSlot();
	}
}

void CInventory::TryActivatePrevSlot()
{
	u16 ActiveSlot		= GetActiveSlot();
	u16 PrevActiveSlot	= GetPrevActiveSlot();
	u16 NextActiveSlot	= GetNextActiveSlot();
	if ((
			(ActiveSlot == NO_ACTIVE_SLOT) ||
			(NextActiveSlot == NO_ACTIVE_SLOT)
		) &&
		(PrevActiveSlot != NO_ACTIVE_SLOT))
	{
		PIItem prev_active_item = ItemFromSlot(PrevActiveSlot);
		if (prev_active_item &&
			!IsSlotBlocked(prev_active_item) &&
			m_slots[PrevActiveSlot].CanBeActivated())
		{
#ifndef MASTER_GOLD
			Msg("Set slots blocked: activating prev slot [%d], Frame[%d]", PrevActiveSlot, Device.dwFrame);
#endif // #ifndef MASTER_GOLD
			Activate(PrevActiveSlot);
			SetPrevActiveSlot(NO_ACTIVE_SLOT);
		}
	}
}

void CInventory::TryDeactivateActiveSlot	()
{
	u16 ActiveSlot		= GetActiveSlot();
	u16 NextActiveSlot	= GetNextActiveSlot();

	if ((ActiveSlot == NO_ACTIVE_SLOT) && (NextActiveSlot == NO_ACTIVE_SLOT))
		return;
	
	PIItem		active_item = (ActiveSlot != NO_ACTIVE_SLOT) ? 
		ItemFromSlot(ActiveSlot) : NULL;
	PIItem		next_active_item = (NextActiveSlot != NO_ACTIVE_SLOT) ?
		ItemFromSlot(NextActiveSlot) : NULL;

	if (active_item &&
		(IsSlotBlocked(active_item) || !m_slots[ActiveSlot].CanBeActivated())
		)
	{
#ifndef MASTER_GOLD
		Msg("Set slots blocked: activating slot [-1], Frame[%d]", Device.dwFrame);
#endif // #ifndef MASTER_GOLD
		ItemFromSlot(ActiveSlot)->DiscardState();
		Activate			(NO_ACTIVE_SLOT);
		SetPrevActiveSlot	(ActiveSlot);
	} else if (next_active_item &&
		(IsSlotBlocked(next_active_item) || !m_slots[NextActiveSlot].CanBeActivated())
		)
	{
		Activate			(NO_ACTIVE_SLOT);
		SetPrevActiveSlot	(NextActiveSlot);
	}
}

void CInventory::BlockSlot(u16 slot_id)
{
	++m_blocked_slots[slot_id];
	
	VERIFY2(m_blocked_slots[slot_id] < 5,
		make_string("blocked slot [%d] overflow").c_str());	
}

void CInventory::UnblockSlot(u16 slot_id)
{
	VERIFY2(m_blocked_slots[slot_id] > 0,
		make_string("blocked slot [%d] underflow").c_str());	
	
	--m_blocked_slots[slot_id];	
}

bool CInventory::IsSlotBlocked(u16 slot_id) const
{
	return m_blocked_slots[slot_id] > 0;
}

bool CInventory::IsSlotBlocked(PIItem const iitem) const
{
	VERIFY(iitem);
	return IsSlotBlocked(iitem->BaseSlot());
}
