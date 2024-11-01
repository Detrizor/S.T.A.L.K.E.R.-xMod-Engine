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
#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "clsid_game.h"
#include "static_cast_checked.hpp"
#include "player_hud.h"
#include "item_container.h"
#include "ActorEffector.h"

//Alundaio
#include "../../xrServerEntities/script_engine.h" 
using namespace ::luabind; 
//-Alundaio

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

bool CInventorySlot::CanBeActivated() const 
{
	return (m_bAct);
};

CInventory::CInventory(CInventoryOwner* owner) : m_pOwner(owner), m_bActors(!!smart_cast<CActor*>(owner))
{
	m_iActiveSlot								= NO_ACTIVE_SLOT;
	m_iNextActiveSlot							= NO_ACTIVE_SLOT;
	m_iPrevActiveSlot							= NO_ACTIVE_SLOT;
	
	m_iReturnPlace								= 0;
	m_iReturnSlot								= 0;
	m_iNextActiveItemID							= u16_max;
	m_iNextLeftItemID							= u16_max;

	//Alundaio: Dynamically create as many slots as we may define in system.ltx
	string256	slot_persistent;
	string256	slot_active;
	xr_strcpy(slot_persistent,"slot_persistent_1");
	xr_strcpy(slot_active,"slot_active_1");
	
	u16 k=1;
	while( pSettings->line_exist("inventory", slot_persistent) && pSettings->line_exist("inventory", slot_active)){
		m_last_slot = k;
		
		m_slots.resize(k+1); //slot+1 because [0] is the inactive slot
		
		m_slots[k].m_bPersistent = !!pSettings->r_BOOL("inventory",slot_persistent);
		m_slots[k].m_bAct = !!pSettings->r_BOOL("inventory",slot_active);
		
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

	m_change_after_deactivate	= false;
	m_bBoltPickUp				= false;
	m_iRuckBlockID				= u16_max;
}

CInventory::~CInventory() 
{
}

void CInventory::Clear()
{
	m_all.clear							();
	m_ruck.clear						();
	m_artefacts.clear					();
	
	for(u16 i=FirstSlot(); i<=LastSlot(); i++)
		m_slots[i].m_pIItem				= NULL;

	for (u8 i = 0; i < m_pockets_count; i++)
		m_pockets[i].clear();

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();
}

void CInventory::Take(CGameObject *pObj, bool strict_placement)
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
	
	u16 place							= (strict_placement) ? pIItem->m_ItemCurrPlace.type : eItemPlaceUndefined;
	u16 slot_id							= pIItem->m_ItemCurrPlace.slot_id;
	pIItem->m_ItemCurrPlace.type		= eItemPlaceUndefined;

	switch (place)
	{
	case eItemPlaceSlot:
		if (slot_id == RIGHT_HAND_SLOT || slot_id == BOTH_HANDS_SLOT)
			m_iNextActiveItemID			= pIItem->object_id();
		else if (slot_id == LEFT_HAND_SLOT)
			m_iNextLeftItemID			= pIItem->object_id();
		else
			Slot						(slot_id, pIItem);
		break;
	case eItemPlacePocket:
		Pocket							(pIItem, slot_id);
		break;
	case eItemPlaceRuck:
		Ruck							(pIItem);
		break;
	case eItemPlaceTrade:
		toTrade							(pIItem);
		break;
	}

	if (pIItem->CurrPlace() == eItemPlaceUndefined)
		tryRuck							(pIItem);
	
	m_pOwner->OnItemTake				(pIItem);
	pIItem->OnTaken						();

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	pIItem->O.processing_deactivate		();
	VERIFY								(pIItem->CurrPlace() != eItemPlaceUndefined);
	
	checkArtefact						(pIItem, true);
}

bool CInventory::DropItem(CGameObject *pObj, bool just_before_destroy, bool dont_create_shell) 
{
	CInventoryItem* pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	VERIFY								(pIItem->m_pInventory);
	VERIFY								(pIItem->m_pInventory==this);
	VERIFY								(pIItem->m_ItemCurrPlace.type!=eItemPlaceUndefined);
	
	pIItem->O.processing_activate		();
	switch (pIItem->CurrPlace())
	{
	case eItemPlaceRuck:
	{
		m_ruck.erase_data				(pIItem);
		break;
	}
	case eItemPlaceSlot:
		if (pIItem == ActiveItem())
		{
			if (m_bActors)
				m_iNextActiveItemID		= u16_max;
			else
				m_iNextActiveSlot		= NO_ACTIVE_SLOT;
		}
		else if (pIItem == LeftItem())
			m_iNextLeftItemID			= u16_max;

		m_slots[pIItem->CurrSlot()].m_pIItem = nullptr;
		break;
	case eItemPlacePocket:
	{
		auto& pocket					= m_pockets[pIItem->CurrPocket()];
		pocket.erase_data				(pIItem);
		break;
	}
	case eItemPlaceTrade:
		m_trade.erase_data				(pIItem);
		break;
	default:
		NODEFAULT;
	}

	m_all.erase_data					(pIItem);
	pIItem->m_pInventory				= nullptr;
	m_pOwner->OnItemDrop				(smart_cast<CInventoryItem*>(pObj), just_before_destroy);

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();
	m_drop_last_frame					= true;
	
	checkArtefact						(pIItem, false);

	if (auto psh = pObj->scast<CPhysicsShellHolder*>())
	{
		Fvector dir						= (m_bActors) ? Actor()->Cameras().Direction() : m_pOwner->O->Direction();
		psh->setActivationSpeedOverride	(dir.mul(5));
	}
	pObj->H_SetParent					(nullptr, dont_create_shell);

	return								true;
}

bool CInventory::trySlot(u16 slot_id, PIItem item)
{
	if (CanPutInSlot(item, slot_id))
	{
		Slot							(slot_id, item);
		return							true;
	}
	return								false;
}

void CInventory::Slot(u16 slot_id, PIItem item)
{
	switch (item->CurrPlace())
	{
	case eItemPlaceSlot:
		if (item->CurrSlot() != slot_id)
		{
			if (GetActiveSlot() == item->CurrSlot())
				Activate				(NO_ACTIVE_SLOT);
			m_slots[item->CurrSlot()].m_pIItem = nullptr;
		}
		break;
	case eItemPlacePocket:
		m_pockets[item->CurrPocket()].erase_data(item);
		break;
	case eItemPlaceRuck:
		m_ruck.erase_data				(item);
		break;
	case eItemPlaceTrade:
		m_trade.erase_data				(item);
		break;
	}

	m_slots[slot_id].m_pIItem			= item;

	SInvItemPlace prev_place			= item->m_ItemCurrPlace;
	item->m_ItemCurrPlace.type			= eItemPlaceSlot;
	item->m_ItemCurrPlace.slot_id		= slot_id;
	item->OnMoveToSlot					(prev_place);
	m_pOwner->OnItemSlot				(item, prev_place);
}

bool CInventory::tryRuck(PIItem item)
{
	if (!m_ruck.contains(item))
	{
		Ruck							(item);
		return							true;
	}
	return								false;
}

void CInventory::Ruck(PIItem item)
{
	switch (item->CurrPlace())
	{
	case eItemPlaceSlot:
		if (GetActiveSlot() == item->CurrSlot())
			Activate					(NO_ACTIVE_SLOT);
		m_slots[item->CurrSlot()].m_pIItem = nullptr;
		break;
	case eItemPlacePocket:
		m_pockets[item->CurrPocket()].erase_data(item);
		break;
	case eItemPlaceTrade:
		m_trade.erase_data				(item);
		break;
	}
	
	m_ruck.push_back					(item);
	
	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	SInvItemPlace prev_place			= item->m_ItemCurrPlace;
	item->m_ItemCurrPlace.type			= eItemPlaceRuck;
	item->OnMoveToRuck					(prev_place);
	m_pOwner->OnItemRuck				(item, prev_place);
}

bool CInventory::tryPocket(PIItem item, u16 pocket_id)
{
	if (CanPutInPocket(item, pocket_id))
	{
		Pocket							(item, pocket_id);
		return							true;
	}
	return								false;
}

void CInventory::Pocket(PIItem item, u16 pocket_id)
{
	switch (item->CurrPlace())
	{
	case eItemPlaceSlot:
		m_slots[item->CurrSlot()].m_pIItem = nullptr;
		break;
	case eItemPlacePocket:
		m_pockets[item->CurrPocket()].erase_data(item);
		break;
	case eItemPlaceRuck:
		m_ruck.erase_data				(item);
		break;
	case eItemPlaceTrade:
		m_trade.erase_data				(item);
		break;
	}
	
	m_pockets[pocket_id].push_back		(item);

	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	SInvItemPlace prev_place			= item->m_ItemCurrPlace;
	item->m_ItemCurrPlace.type			= eItemPlacePocket;
	item->m_ItemCurrPlace.slot_id		= pocket_id;
	item->OnMoveToRuck					(prev_place);
	m_pOwner->OnItemRuck				(item, prev_place);
}

void CInventory::toTrade(PIItem item)
{
	switch (item->CurrPlace())
	{
	case eItemPlaceSlot:
		if (InHands(item))
			((item->BaseSlot() == LEFT_HAND_SLOT) ? m_iNextLeftItemID : m_iNextActiveItemID) = u16_max;
		m_slots[item->CurrSlot()].m_pIItem = nullptr;
		break;
	case eItemPlacePocket:
		m_pockets[item->CurrPocket()].erase_data(item);
		break;
	case eItemPlaceRuck:
		m_ruck.erase_data				(item);
		break;
	}
	
	m_trade.push_back					(item);
	
	CalcTotalWeight						();
	CalcTotalVolume						();
	InvalidateState						();

	SInvItemPlace prev_place			= item->m_ItemCurrPlace;
	item->m_ItemCurrPlace.type			= eItemPlaceTrade;
	item->OnMoveToRuck					(prev_place);
	m_pOwner->OnItemRuck				(item, prev_place);
}

PIItem CInventory::ItemFromSlot(u16 slot) const
{
	return								(slot == NO_ACTIVE_SLOT) ? nullptr : m_slots[slot].m_pIItem;
}

void CInventory::SendActionEvent(u16 cmd, u32 flags) 
{
	if (!m_bActors)
		return;
	
	CActor* pActor			= smart_cast<CActor*>(m_pOwner);
	NET_Packet				P;
	pActor->u_EventGen		(P, GE_INV_ACTION, pActor->ID());
	P.w_u16					(cmd);
	P.w_u32					(flags);
	P.w_s32					(pActor->GetZoomRndSeed());
	P.w_s32					(pActor->GetShotRndSeed());
	pActor->u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
}

bool CInventory::Action(u16 cmd, u32 flags) 
{
	if (m_bActors)
	{
		CActor* pActor = smart_cast<CActor*>(m_pOwner);
		switch(cmd)
		{
			case kWPN_FIRE:
				pActor->SetShotRndSeed();
				break;
			case kWPN_ZOOM:
				pActor->SetZoomRndSeed();
				break;
		}
	}
	
	if (g_pGameLevel && OnClient() && m_bActors)
	{
		switch(cmd)
		{
		case kDROP:{
			SendActionEvent(cmd, flags);
			return true;
			}break;
		case kWPN_RELOAD:
		case kWPN_FIRE:
		case kWPN_FUNC:
		case kWPN_FIREMODE_NEXT:
		case kWPN_FIREMODE_PREV:
		case kWPN_ZOOM:
		case kTORCH:
		case kNIGHT_VISION:
			SendActionEvent(cmd, flags);
			break;
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
				ActivateItem				(slot_item);
			else
			{
				PIItem active_item			= ActiveItem();
				if (active_item && (active_item->BaseSlot() == slot || (active_item->BaseSlot() == PRIMARY_SLOT && slot == SECONDARY_SLOT)))
					ActivateItem			(active_item, eItemPlaceSlot, slot);
			}
		}
		break;
	}

	if (b_send_event && g_pGameLevel && OnClient() && m_bActors)
		SendActionEvent(cmd, flags);

	return false;
}

void CInventory::Activate(u16 slot, bool bForce)
{
	if (!OnServer() || m_bActors)
		return;
	R_ASSERT2(slot <= LastSlot(), "wrong slot number");

	PIItem tmp_item = nullptr;
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
		else if (slot == GRENADE_SLOT)	//fake for grenade
			if (PIItem gr = SameSlot(GRENADE_SLOT, nullptr))
				Slot(GRENADE_SLOT, gr);
	}
	else if (slot == NO_ACTIVE_SLOT || tmp_item)	//активный слот задействован
	{
		PIItem active_item = ActiveItem();
		if (active_item && !bForce)
		{
			CHudItem* tempItem = active_item->cast_hud_item();
			R_ASSERT2(tempItem, active_item->object().cNameSect().c_str());
			tempItem->deactivateItem();
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

void CInventory::ActivateItem(PIItem item, u16 return_place, u16 return_slot)
{
	bool force_deactivating				= false;
	if (!item)
	{
		item							= ActiveItem();
		force_deactivating				= true;
		if (!item)
			return;
	}

	u16 hand							= item->HandSlot();
	bool can_set						= (item->parent_id() == m_pOwner->object_id());
	PIItem active_item					= ActiveItem();
	
	m_iReturnPlace						= return_place;
	m_iReturnSlot						= return_slot;

	if (active_item)
	{
		bool deactivating				= (item == active_item);
		bool interferes					= (hand != LEFT_HAND_SLOT || active_item->HandSlot() == BOTH_HANDS_SLOT);
		if (deactivating || interferes)
		{
			u16 active_item_base_slot	= active_item->BaseSlot();
			PIItem next_active_item		= (deactivating) ? nullptr : item;
			u16 next_item_cur_slot		= (next_active_item) ? next_active_item->CurrSlot() : 0;

			if (m_iReturnPlace);
			else if (CanPutInSlot(active_item, active_item_base_slot))
			{
				m_iReturnPlace			= eItemPlaceSlot;
				m_iReturnSlot			= active_item_base_slot;
			}
			else if (active_item_base_slot == PRIMARY_SLOT && CanPutInSlot(active_item, SECONDARY_SLOT))
			{
				m_iReturnPlace			= eItemPlaceSlot;
				m_iReturnSlot			= SECONDARY_SLOT;
			}
			else if (next_active_item && (next_item_cur_slot == active_item_base_slot || (active_item_base_slot == PRIMARY_SLOT && next_item_cur_slot == SECONDARY_SLOT)))
			{
				m_iReturnPlace			= eItemPlaceSlot;
				m_iReturnSlot			= next_item_cur_slot;
			}
			else
			{
				m_iReturnPlace			= 0;
				auto section			= item->Section(true);
				for (int i = 0; i < m_pockets_count; ++i)
				{
					if (section == ACTOR_DEFS::g_quick_use_slots[i] && CanPutInPocket(item, i))
					{
						m_iReturnPlace	= eItemPlacePocket;
						m_iReturnSlot	= i;
						break;
					}
				}
				if (!m_iReturnPlace)
				{
					for (int i = 0; i < m_pockets_count; ++i)
					{
						if (CanPutInPocket(active_item, i))
						{
							m_iReturnPlace = eItemPlacePocket;
							m_iReturnSlot = i;
							break;
						}
					}
				}
			}

			auto hi						= active_item->O.scast<CHudItem*>();
			u16 ret_slot				= (m_iReturnPlace == eItemPlaceSlot) ? m_iReturnSlot : 0;
			if (deactivating)
			{
				if (hi->IsHiding() && !force_deactivating)
					hi->activateItem	();
				else if (!hi->IsHidden())
				{
					if (m_iReturnPlace)
					{
						hi->deactivateItem(ret_slot);
						can_set			= false;
					}
					else
						Ruck			(active_item);
				}
			}
			else if (interferes)
			{
				if (m_iReturnPlace)
				{
					hi->deactivateItem	(ret_slot);
					m_iNextActiveItemID	= u16_max;
					can_set				= false;
				}
				else
					Ruck				(active_item);
			}
		}
	}
	
	if (auto left_item = LeftItem())
	{
		auto hi							= left_item->O.scast<CHudItem*>();
		if (item == left_item)
		{
			if (hi->IsHiding())
				hi->activateItem		();
			else if (!hi->IsHidden())
				hi->deactivateItem		();
			can_set						= false;
		}
		else if (hand != RIGHT_HAND_SLOT)
		{
			hi->deactivateItem			();
			m_iNextLeftItemID			= u16_max;
			can_set						= false;
		}
	}

	u16& tmp							= (hand == LEFT_HAND_SLOT) ? m_iNextLeftItemID : m_iNextActiveItemID;
	tmp									= (tmp == item->object_id()) ? u16_max : item->object_id();

	if (can_set && item != active_item)
		Slot							(hand, item);
}

CInventoryItem* CInventory::get_item_by_id(ALife::_OBJECT_ID tObjectID)
{
	if (tObjectID != u16_max)
		for (auto item : m_all)
			if (item->object_id() == tObjectID)
				return					item;
	return								nullptr;
}

void CInventory::update_actors()
{
	PIItem active_item					= ActiveItem();
	u16 active_item_id					= (active_item) ? active_item->object_id() : u16_max;
	bool active_swap					= (active_item_id != m_iNextActiveItemID);

	PIItem left_item					= LeftItem();
	u16 left_item_id					= (left_item) ? left_item->object_id() : u16_max;
	bool left_swap						= (left_item_id != m_iNextLeftItemID);

	if (!active_swap && !left_swap)
		return;

	PIItem next_active_item				= get_item_by_id(m_iNextActiveItemID);
	PIItem next_left_item				= get_item_by_id(m_iNextLeftItemID);
	bool can_set						= true;

	if (active_item && (active_swap || (next_left_item && active_item->HandSlot() == BOTH_HANDS_SLOT)))
	{
		auto hi							= active_item->O.scast<CHudItem*>();
		if (hi->IsHidden())
		{
			if (m_iReturnPlace == eItemPlaceSlot)
			{
				if (next_active_item && next_active_item->CurrSlot() == m_iReturnSlot)
				{
					Ruck				(active_item);
					Slot				(next_active_item->HandSlot(), next_active_item);
					can_set				= false;
				}
				Slot					(m_iReturnSlot, active_item);
			}
			else
				Pocket					(active_item, m_iReturnSlot);
			m_iReturnPlace				= 0;
			m_iReturnSlot				= 0;
		}
		else
			can_set						= false;
	}

	if (left_item && (left_swap || (next_active_item && next_active_item->HandSlot() == BOTH_HANDS_SLOT)))
	{
		auto hi							= left_item->O.scast<CHudItem*>();
		if (hi->IsHidden())
		{
			if (m_iReturnPlace == eItemPlaceSlot)
				Slot					(m_iReturnSlot, left_item);
			else
				Pocket					(left_item, m_iReturnSlot);
			m_iReturnPlace				= 0;
			m_iReturnSlot				= 0;
		}
		else
			can_set						= false;
	}

	if (can_set)
	{
		if (active_swap && next_active_item)
			Slot						(next_active_item->HandSlot(), next_active_item);
		if (left_swap && next_left_item)
			Slot						(LEFT_HAND_SLOT, next_left_item);
	}
}

void CInventory::Update()
{
	if (OnServer())
	{
		if (m_bActors)
			update_actors();
		else
		{
			if (m_iActiveSlot != m_iNextActiveSlot)
			{
				if (auto active_item = ActiveItem())
				{
					auto hi = active_item->cast_hud_item();
					if (!hi->IsHidden())
					{
						if (hi->GetState() == CHUDState::eIdle)
							hi->deactivateItem(active_item->BaseSlot());

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
							tmp_next_active->ActivateItem(tmp_next_active->CurrSlot());
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

bool CInventory::Bag(PIItem item)
{
	if (item->object_id() == m_iRuckBlockID)
	{
		m_iRuckBlockID				= u16_max;
		return						false;
	}
	PIItem ai						= ActiveItem();
	if (!ai)
		return						false;
	MContainer* container			= ai->O.getModule<MContainer>();
	if (!container || !container->CanTakeItem(item))
		return						false;
	item->O.transfer				(container->O.ID());
	return							true;
}

PIItem CInventory::ActiveItem() const
{
	if (!m_bActors)
		return					(m_iActiveSlot == NO_ACTIVE_SLOT) ? NULL : ItemFromSlot(m_iActiveSlot);

	PIItem both_hands_item		= ItemFromSlot(BOTH_HANDS_SLOT);
	PIItem right_hand_item		= ItemFromSlot(RIGHT_HAND_SLOT);
	return						(both_hands_item) ? both_hands_item : right_hand_item;
}

PIItem CInventory::LeftItem(bool with_activation) const
{
	PIItem left_item				= ItemFromSlot(LEFT_HAND_SLOT);
	return							(left_item && with_activation && m_iNextLeftItemID != left_item->object_id()) ? NULL : left_item;
}

u16 CInventory::GetActiveSlot() const
{
	if (m_bActors)
	{
		PIItem active_item		= ActiveItem();
		return					(active_item) ? active_item->BaseSlot() : NO_ACTIVE_SLOT;
	}
	return						m_iActiveSlot;
}

void CInventory::UpdateDropTasks()
{
	for (u16 i = FirstSlot(); i <= LastSlot(); ++i)
		if (auto itm = ItemFromSlot(i))
			UpdateDropItem				(itm);
	
	for (auto item : m_ruck)
		if (!process_item(item))
			UpdateDropItem				(item);

	if (m_drop_last_frame)
	{
		m_drop_last_frame				= false;
		m_pOwner->OnItemDropUpdate		();
	}
}

bool CInventory::process_item(PIItem item)
{
	if (!item->Weight())
		return							false;

	if (trySlot(item->BaseSlot(), item))
		return							true;

	if (!m_bActors)
		return							false;

	for (u8 i = 0; i < m_pockets_count; ++i)
		if (item->Section(true) == ACTOR_DEFS::g_quick_use_slots[i] && tryPocket(item, i))
			return						true;

	if (item->BaseSlot() == PRIMARY_SLOT && trySlot(SECONDARY_SLOT, item))
		return							true;

	if (Bag(item))
		return							true;

	for (u8 i = 0; i < m_pockets_count; ++i)
		if (tryPocket(item, i))
			return						true;

	if (CanPutInSlot(item, item->HandSlot()) && item->object_id() != m_iNextActiveItemID && item->object_id() != m_iNextLeftItemID)
	{
		ActivateItem					(item);
		return							true;
	}

	item->O.transfer					(u16_max, true);
	return								true;
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
PIItem CInventory::SameSlot(const u16 slot, PIItem pIItem, LPCSTR section) const
{
	if (slot == NO_ACTIVE_SLOT)
		return							nullptr;

	for (auto item : m_ruck)
		if (item->BaseSlot() == slot && item != pIItem && xr_strcmp(item->m_section_id.c_str(), section))
			return						item;

	for (u8 i = 0; i < m_pockets_count; ++i)
	{
		for (auto item : m_pockets[i])
		{
			if (item->BaseSlot() == slot && item != pIItem && xr_strcmp(item->m_section_id.c_str(), section))
				return					item;
		}
	}

	return								nullptr;
}

//найти в инвенторе вещь с указанным именем
PIItem CInventory::Get(LPCSTR name, int pocket_id, bool full) const
{
	if (pocket_id == -1)
	{
		for (TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it)
		{
			PIItem pIItem	= *it;
			if ((full ? pIItem->Section(full) : pIItem->m_section_id) == name)
				return		pIItem;
		}
	}

	if (pocket_id != -1)
	{
		bool hands_free				= !ItemFromSlot(RIGHT_HAND_SLOT) && !ItemFromSlot(BOTH_HANDS_SLOT);
		float best_fill				= float(hands_free);
		PIItem best_item			= NULL;
		for (TIItemContainer::const_iterator it = m_pockets[pocket_id].begin(), it_e = m_pockets[pocket_id].end(); it != it_e; ++it)
		{
			const PIItem item		= *it;
			float fill				= item->GetFill();
			if (item->Section(full) == name && (hands_free ? fLessOrEqual(fill, best_fill) : fMoreOrEqual(fill, best_fill)))
			{
				best_fill			= fill;
				best_item			= item;
			}
		}
		if (best_item)
			return					best_item;
	}
	else
	{
		for (u8 i = 0, i_e =  m_pockets_count; i < i_e; i++)
		{
			for (TIItemContainer::const_iterator it = m_pockets[i].begin(), it_e = m_pockets[i].end(); it != it_e; ++it)
			{
				const PIItem item		= *it;
				if (item->Section(full) == name)
					return				item;
			}
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

//скушать предмет 
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
bool CInventory::Eat(PIItem pIItem)
{
	//устанаовить съедобна ли вещь
	CEatableItem* pItemToEat	= smart_cast<CEatableItem*>(pIItem);
	if (!pItemToEat)
		return					false;

	MAmountable* amt			= pIItem->O.getModule<MAmountable>();
	if (amt && amt->Empty())
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
	if (pInventory != IO->m_inventory.get())
		return					false;
	if (pItemToEat->H_Parent()->ID() != entity_alive->ID())
		return					false;

	if (!pItemToEat->UseBy(entity_alive))
		return					false;

	::luabind::functor<bool>	funct;
	if (ai().script_engine().functor("_G.CInventory__eat", funct))
		if (!funct(smart_cast<CGameObject*>(pItemToEat->H_Parent())->lua_game_object(), (smart_cast<CGameObject*>(pIItem))->lua_game_object()))
			return false;

	if (amt)
		amt->Deplete			();
	else
		pIItem->O.DestroyObject	();

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
	if ( pInventory != IO->m_inventory.get() )	return false;
	if ( pItemToEat->H_Parent()->ID() != entity_alive->ID() )
		return false;
	
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

bool CInventory::CanPutInSlot(PIItem pIItem, u16 slot_id) const
{
	if (!m_bSlotsUseful || slot_id == NO_ACTIVE_SLOT || ItemFromSlot(slot_id))
		return						false;

	if (!GetOwner()->CanPutInSlot(pIItem, slot_id))
		return						false;
	
	if (slot_id == HELMET_SLOT)
		if (auto outfit = m_pOwner->GetOutfit())
			if (!outfit->bIsHelmetAvaliable)
				return				false;

	if ((slot_id == LEFT_HAND_SLOT || slot_id == RIGHT_HAND_SLOT) && ItemFromSlot(BOTH_HANDS_SLOT))
		return						false;

	if (slot_id == BOTH_HANDS_SLOT && (ItemFromSlot(LEFT_HAND_SLOT) || ItemFromSlot(RIGHT_HAND_SLOT)))
		return						false;

	return							true;
}

bool CInventory::CanPutInPocket(PIItem pIItem, u16 pocket_id) const
{
	if (!m_bActors || !PocketPresent(pocket_id))
		return							false;

	float volume						= 0.f;
	for (auto& item : m_pockets[pocket_id])
		volume							+= item->Volume();
	float max_volume					= m_pOwner->GetOutfit()->m_pockets[pocket_id];

	return								(volume < max_volume && volume + pIItem->Volume() < max_volume + .1f);
}

bool CInventory::PocketPresent(u16 pocket_id) const
{
	CCustomOutfit* outfit				= m_pOwner->GetOutfit();
	return								(outfit && pocket_id < outfit->m_pockets.size());
}

void CInventory::emptyPockets()
{
	for (auto& pocket : m_pockets)
		for (auto item : pocket)
			process_item				(item);
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
				::luabind::functor<bool> funct;
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
						::luabind::functor<bool> funct;
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

void CInventory::checkArtefact(PIItem item, bool take)
{
	if (auto artefact = item->O.scast<CArtefact*>())
	{
		auto& it						= m_artefacts.find(artefact);
		if (it != m_artefacts.end())
		{
			if (!take)
				m_artefacts.erase		(it);
		}
		else if (take)
			m_artefacts.push_back		(artefact);
	}
	else if (auto con = item->O.getModule<MContainer>())
		for (auto I : con->Items())
			checkArtefact				(I, take);
}
