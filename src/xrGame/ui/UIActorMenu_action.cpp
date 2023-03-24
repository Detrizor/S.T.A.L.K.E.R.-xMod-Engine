////////////////////////////////////////////////////////////////////////////
//	Module 		: UIActorMenu_action.cpp
//	Created 	: 14.10.2008
//	Author		: Evgeniy Sokolov (sea)
//	Description : UI ActorMenu actions implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIActorStateInfo.h"
#include "../actor.h"
#include "../uigamesp.h"
#include "../inventory.h"
#include "../inventory_item.h"
#include "../InventoryBox.h"
#include "object_broker.h"
#include "UIInventoryUtilities.h"
#include "game_cl_base.h"

#include "UICursor.h"
#include "UICellItem.h"
#include "UICharacterInfo.h"
#include "UIItemInfo.h"
#include "UIDragDropListEx.h"
#include "UIInventoryUpgradeWnd.h"
#include "UI3tButton.h"
#include "UIBtnHint.h"
#include "UIMessageBoxEx.h"
#include "UIPropertiesBox.h"
#include "UIMainIngameWnd.h"
#include "../IItemContainer.h"
#include "ActorBackpack.h"

using namespace luabind; //Alundaio

bool  CUIActorMenu::AllowItemDrops(EDDListType from, EDDListType to)
{
	xr_vector<EDDListType>& v = m_allowed_drops[to];
	xr_vector<EDDListType>::iterator it = std::find(v.begin(), v.end(), from);

	return(it!=v.end());
}

class CUITrashIcon :public ICustomDrawDragItem
{
	CUIStatic			m_icon;
public:
	CUITrashIcon		(LPCSTR texture)
	{
		m_icon.SetWndSize		(Fvector2().set(25.f * UI().get_current_kx(), 25.0f));
		m_icon.SetStretchTexture(true);
		m_icon.InitTexture		(texture);
	}
	virtual void		OnDraw		(CUIDragItem* drag_item)
	{
		Fvector2 pos			= drag_item->GetWndPos();
		Fvector2 icon_sz		= m_icon.GetWndSize();
		Fvector2 drag_sz		= drag_item->GetWndSize();

		pos.x			-= icon_sz.x;
		pos.y			+= drag_sz.y;

		m_icon.SetWndPos(pos);
//		m_icon.SetWndSize(sz);
		m_icon.Draw		();
	}
};

void CUIActorMenu::OnDragItemOnTrash(CUIDragItem* di, bool b_receive)
{
	CUICellItem* ci					= di->ParentItem();
	if (ci->OwnerList() != m_pTrashList && ci->m_pData)
	{
		if (b_receive)
			di->SetCustomDraw		(xr_new<CUITrashIcon>("ui_inGame2_inv_trash"));
		else
			di->SetCustomDraw		(NULL);
	}
}

void CUIActorMenu::OnDragItemOnPocket(CUIDragItem* di, bool b_receive)
{
	CUICellItem* ci						= di->ParentItem();
	CUIDragDropListEx* list				= di->BackList();
	if (ci->OwnerList() != list && ci->m_pData)
	{
		u16 idx							= GetPocketIdx(list);
		if (b_receive && m_pActorInv->PocketPresent(idx))
		{
			if (m_pActorInv->CanPutInPocket((PIItem)ci->m_pData, idx))
				di->SetCustomDraw		(xr_new<CUITrashIcon>("ui_inGame2_pocket_fit"));
			else
				di->SetCustomDraw		(xr_new<CUITrashIcon>("ui_inGame2_pocket_not_fit"));
		}
		else
			di->SetCustomDraw			(NULL);
	}
}

extern bool TryCustomUse(PIItem item);
bool CUIActorMenu::OnItemDrop(CUICellItem* itm)
{
	InfoCurItem							(NULL);
	CUIDragDropListEx* old_owner		= itm->OwnerList();
	CUIDragDropListEx* new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	
	CUIDragDropListEx* left_hand		= m_pInvList[LEFT_HAND_SLOT];
	CUIDragDropListEx* right_hand		= m_pInvList[RIGHT_HAND_SLOT];
	CUIDragDropListEx* both_hands		= m_pInvList[BOTH_HANDS_SLOT];
	if (itm->m_pData && (new_owner == left_hand || new_owner == right_hand || new_owner == both_hands))
	{
		if (old_owner == left_hand || old_owner == right_hand || old_owner == both_hands)
			return						true;
		u16 hand_slot					= PIItem(itm->m_pData)->HandSlot();
		if (both_hands->ItemsCount() == 1 || hand_slot == BOTH_HANDS_SLOT)
			new_owner					= both_hands;
		else if (!left_hand->ItemsCount() && !right_hand->ItemsCount())
			new_owner					= m_pInvList[hand_slot];
		else if (new_owner == both_hands)
			new_owner					= left_hand;
	}

	if (!old_owner || !new_owner )
		return false;

	EDDListType t_new		= GetListType(new_owner);
	EDDListType t_old		= GetListType(old_owner);

	if ( !AllowItemDrops(t_old, t_new) )
	{
		Msg("incorrect action [%d]->[%d]",t_old, t_new);
		return true;
	}

	if (old_owner == new_owner)
	{
		//Alundaio: Here we export the action of dragging one inventory item ontop of another! 
		luabind::functor<bool> funct1;
		if (ai().script_engine().functor("actor_menu_inventory.CUIActorMenu_OnItemDropped", funct1))
		{
			//If list only has 1 item, get it, otherwise try to get item at current drag position
			CUICellItem* _citem = (new_owner->ItemsCount() == 1) ? new_owner->GetItemIdx(0) : NULL;
			if (!_citem)
			{ 
				CUICellContainer* c = old_owner->GetContainer();
				Ivector2 c_pos = c->PickCell(old_owner->GetDragItemPosition());
				if (c->ValidCell(c_pos))
				{
					CUICell& ui_cell = c->GetCellAt(c_pos);
					if (!ui_cell.Empty())
						_citem = ui_cell.m_item;
				}
			}

			PIItem _iitem = _citem ? (PIItem)_citem->m_pData : NULL;

			CGameObject* GO1 = smart_cast<CGameObject*>(CurrentIItem());
			CGameObject* GO2 = _iitem ? smart_cast<CGameObject*>(_iitem) : NULL;
			if (funct1(GO1 ? GO1->lua_game_object() : (0), GO2 ? GO2->lua_game_object() : (0), (int)t_old, (int)t_new) == false)
				return false;
		}
		//-Alundaio
		return false;
	}

	PIItem item							= CurrentIItem();
	bool gear							= smart_cast<CCustomOutfit*>(item) || smart_cast<CHelmet*>(item) || smart_cast<CBackpack*>(item);
	bool gear_equipped					= gear && item->CurrSlot() == item->BaseSlot();
	switch(t_new)
	{
	case iActorSlot:
		u16								slot_to_place;
		if (CanSetItemToList(item, new_owner, slot_to_place))
		{
			if (gear_equipped && slot_to_place == item->HandSlot() || gear && slot_to_place == item->BaseSlot())
				TryCustomUse			(item);
			else
				ToSlot					(itm, slot_to_place);
		}
		break;
	case iActorPocket:
		if (!gear_equipped)
		{
			Ivector2 cells				= new_owner->CellsCapacity();
			Ivector2 grid				= itm->GetGridSize();
			if (cells.x >= grid.x)
				ToPocket				(itm, true, GetPocketIdx(new_owner));
		}
		break;
	case iActorBag:
		if (!gear_equipped)
			ToBag						(itm, true);
		break;
	case iTrashSlot:
		if (gear_equipped)
			break;
		if (t_old == iDeadBodyBag || t_old == iActorBag)
		{
			ToRuck						(item);
			m_pActorInv->m_iToDropID	= item->object_id();
		}
		else
			item->Transfer				();
		SetCurrentItem					(NULL);
		break;
	case iDeadBodyBag:
		if (!gear_equipped)
			ToDeadBodyBag				(itm, true);
		break;
	case iActorTrade:
		if (!gear_equipped)
			ToActorTrade				(itm, true);
		break;
	case iPartnerTrade:
		ToPartnerTrade					(itm, true);
		break;
	case iPartnerTradeBag:
		ToPartnerTradeBag				(itm, true);
		break;
	};

	OnItemDropped						(CurrentIItem(), new_owner, old_owner);

	//Alundaio: Here we export the action of dragging one inventory item ontop of another! 
	luabind::functor<bool> funct1;
	if (ai().script_engine().functor("actor_menu_inventory.CUIActorMenu_OnItemDropped", funct1))
	{
		//If list only has 1 item, get it, otherwise try to get item at current drag position
		CUICellItem* _citem = (new_owner->ItemsCount() == 1) ? new_owner->GetItemIdx(0) : NULL;
		if (!_citem)
		{
			CUICellContainer* c = old_owner->GetContainer();
			Ivector2 c_pos = c->PickCell(old_owner->GetDragItemPosition());
			if (c->ValidCell(c_pos))
			{
				CUICell& ui_cell = c->GetCellAt(c_pos);
				if (!ui_cell.Empty())
					_citem = ui_cell.m_item;
			}
		}

		PIItem _iitem = _citem ? (PIItem)_citem->m_pData : NULL;

		CGameObject* GO1 = smart_cast<CGameObject*>(CurrentIItem());
		CGameObject* GO2 = _iitem ? smart_cast<CGameObject*>(_iitem) : NULL;
		if (funct1(GO1 ? GO1->lua_game_object() : (0), GO2 ? GO2->lua_game_object() : (0), (int)t_old, (int)t_new) == false)
			return false;
	}
	//-Alundaio

	UpdateConditionProgressBars();
	UpdateItemsPlace();

	return true;
}

bool CUIActorMenu::OnItemStartDrag(CUICellItem* itm)
{
	InfoCurItem( NULL );
	return false; //default behaviour
}

bool CUIActorMenu::OnItemDbClick(CUICellItem* itm)
{
	SetCurrentItem					(itm);
	InfoCurItem						(NULL);
	CUIDragDropListEx* old_owner	= itm->OwnerList();
	EDDListType t_old				= GetListType(old_owner);
	PIItem item						= (PIItem)itm->m_pData;

	switch (t_old)
	{
	case iActorSlot:
	{
		u16 slot_id					= item->CurrSlot();
		if ((slot_id == OUTFIT_SLOT || slot_id == HELMET_SLOT || slot_id == BACKPACK_SLOT) && TryCustomUse(item));
		else if (m_currMenuMode == mmTrade && ToActorTrade(itm, false));
		else if (m_currMenuMode == mmDeadBodySearch && ToDeadBodyBag(itm, false));
		else if (slot_id == item->HandSlot() && ToSlot(itm, item->BaseSlot(), true));
		else if (slot_id != item->HandSlot() && ToSlot(itm, item->HandSlot()));
		else if (GetBag() && ToBag(itm, false));
		else if (TryCustomUse(item));
		else ToRuck(item);
		break;
	}
	case iActorPocket:
		if (m_currMenuMode == mmTrade && ToActorTrade(itm, false));
		else if (m_currMenuMode == mmDeadBodySearch && ToDeadBodyBag(itm, false));
		else if (ToSlot(itm, item->BaseSlot(), true));
		else if (ToSlot(itm, item->HandSlot()));
		else if (GetBag() && ToBag(itm, false));
		else if (TryCustomUse(item));
		else item->Transfer();
		break;
	case iActorBag:
		if (m_currMenuMode == mmTrade && ToActorTrade(itm, false));
		else if (m_currMenuMode == mmDeadBodySearch && ToDeadBodyBag(itm, false));
		else
		{
			m_pActorInv->m_iRuckBlockID = item->object_id();
			ToRuck(item);
		}
		break;
	case iTrashSlot:
		if (m_currMenuMode == mmDeadBodySearch && ToDeadBodyBag(itm, false));
		else if (GetBag() && ToBag(itm, false));
		else ToRuck(item);
		break;
	case iDeadBodyBag:
		if (GetBag() && ToBag(itm, false));
		else ToRuck(item);
		break;
	case iActorTrade:
		if (GetBag() && ToBag(itm, false));
		else ToRuck(item);
		break;
	case iPartnerTrade:
		ToPartnerTradeBag			(itm, false);
		break;
	case iPartnerTradeBag:
		ToPartnerTrade				(itm, false);
		break;
	}; //switch 

	UpdateConditionProgressBars		();
	UpdateItemsPlace				();

	return true;
}

bool CUIActorMenu::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem		(itm);
	InfoCurItem			(NULL);
	m_item_info_view	= false;
	return				false;
}

bool CUIActorMenu::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem( itm );
	InfoCurItem( NULL );
	ActivatePropertiesBox();
	m_item_info_view = false;
	return false;
}

bool CUIActorMenu::OnItemFocusReceive(CUICellItem* itm)
{
	InfoCurItem( NULL );
	m_item_info_view = true;

	itm->m_selected = true;
	set_highlight_item( itm );

	luabind::functor<bool> funct1;
	if (ai().script_engine().functor("actor_menu_inventory.CUIActorMenu_OnItemFocusReceive", funct1))
	{
		PIItem _iitem = (PIItem)itm->m_pData;

		CGameObject* GO = _iitem ? smart_cast<CGameObject*>(_iitem) : NULL;
		if (GO)
			funct1(GO->lua_game_object());
	}

	return true;
}

bool CUIActorMenu::OnItemFocusLost(CUICellItem* itm)
{
	if ( itm )
	{
		itm->m_selected = false;
	}
	InfoCurItem( NULL );
	clear_highlight_lists();

	luabind::functor<bool> funct1;
	if (ai().script_engine().functor("actor_menu_inventory.CUIActorMenu_OnItemFocusLost", funct1))
	{
		PIItem _iitem = (PIItem)itm->m_pData;

		CGameObject* GO = _iitem ? smart_cast<CGameObject*>(_iitem) : NULL;
		if (GO)
			funct1(GO->lua_game_object());
	}

	return true;
}

bool CUIActorMenu::OnItemFocusedUpdate(CUICellItem* itm)
{
	if ( itm )
	{
		itm->m_selected = true;
		if ( m_highlight_clear )
		{
			set_highlight_item( itm );
		}
	}
	VERIFY( m_ItemInfo );
	if ( Device.dwTimeGlobal < itm->FocusReceiveTime() + m_ItemInfo->delay )
	{
		return true; //false
	}
	if ( CUIDragDropListEx::m_drag_item || m_UIPropertiesBox->IsShown() || !m_item_info_view )
	{
		return true;
	}	

	InfoCurItem( itm );
	return true;
}

bool CUIActorMenu::OnMouseAction( float x, float y, EUIMessages mouse_action )
{
	inherited::OnMouseAction( x, y, mouse_action );
	return true; // no click`s
}

bool CUIActorMenu::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	InfoCurItem( NULL );
	if ( is_binded(kSPRINT_TOGGLE, dik) )
	{
		if ( WINDOW_KEY_PRESSED == keyboard_action )
		{
			OnPressUserKey();
		}
		return true;
	}

	if ((is_binded(kUSE, dik) || is_binded(kINVENTORY, dik)) && m_currMenuMode != mmInventory)
	{
		if (WINDOW_KEY_PRESSED == keyboard_action)
		{
			g_btnHint->Discard();
			HideDialog();
		}
		return true;
	}

	if (is_binded(kINVENTORY, dik) || is_binded(kQUIT, dik))
	{
		if (WINDOW_KEY_PRESSED == keyboard_action)
		{
			g_btnHint->Discard();
			HideDialog();
		}
		return true;
	}

	return inherited::OnKeyboardAction(dik, keyboard_action);
}

void CUIActorMenu::OnPressUserKey()
{
	switch ( m_currMenuMode )
	{
	case mmUndefined:		break;
	case mmInventory:		break;
	case mmTrade:			
//		OnBtnPerformTrade( this, 0 );
		break;
	case mmUpgrade:			
		TrySetCurUpgrade();
		break;
	case mmDeadBodySearch:	
		TakeAllFromPartner( this, 0 );
		break;
	default:
		R_ASSERT(0);
		break;
	}
}

void CUIActorMenu::OnBtnExitClicked(CUIWindow* w, void* d)
{
	g_btnHint->Discard();
	HideDialog();
}

void CUIActorMenu::OnMesBoxYes( CUIWindow*, void* )
{
	switch( m_currMenuMode )
	{
	case mmUndefined:
		break;
	case mmInventory:
		break;
	case mmTrade:
		break;
	case mmUpgrade:
		if ( m_repair_mode )
		{
			RepairEffect_CurItem();
			m_repair_mode = false;
		}
		else
		{
			m_pUpgradeWnd->OnMesBoxYes();
		}
		break;
	case mmDeadBodySearch:
		break;
	default:
		R_ASSERT(0);
		break;
	}
	UpdateItemsPlace();
}

void CUIActorMenu::OnMesBoxNo(CUIWindow*, void*)
{
	switch(m_currMenuMode)
	{
	case mmUndefined:
		break;
	case mmInventory:
		break;
	case mmTrade:
		break;
	case mmUpgrade:
		m_repair_mode = false;
		break;
	case mmDeadBodySearch:
		break;
	default:
		R_ASSERT(0);
		break;
	}
	UpdateItemsPlace();
}
