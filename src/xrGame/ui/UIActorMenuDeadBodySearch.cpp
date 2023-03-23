#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIDragDropListEx.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "UI3tButton.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIFrameLineWnd.h"

#include "xrMessages.h"
#include "../alife_registry_wrappers.h"
#include "../GameObject.h"
#include "../InventoryOwner.h"
#include "../Inventory.h"
#include "../Inventory_item.h"
#include "../InventoryBox.h"
#include "../string_table.h"
#include "../ai/monsters/BaseMonster/base_monster.h"

void move_item_from_to (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
	CGameObject::u_EventGen					(P, GE_TRADE_SELL, from_id);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen					(P, GE_TRADE_BUY, to_id);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);
}

// -------------------------------------------------------------------------------------------------

void CUIActorMenu::InitDeadBodySearchMode()
{
	m_pDeadBodyBagList->Show		(true);
	m_PartnerWeightInfo->Show		(true);
	m_PartnerWeight->Show			(true);
	m_PartnerVolumeInfo->Show		(true);
	m_PartnerVolume->Show			(true);

	if (m_pInvBox)
	{
		m_takeall_button->Show								(true);
		shared_str section									= m_pInvBox->cNameSect();
		bool virtual_box									= !xr_strcmp(*section, "virtual_box");
		m_takeall_button->Enable							(pSettings->line_exist(section, "pick_up_section") || virtual_box);
		shared_str text										= (virtual_box) ? "st_close" : "st_pick_up";
		m_takeall_button->TextItemControl()->SetText		(*CStringTable().translate(text));
		text.printf											("%s_hint", *text);
		m_takeall_button->m_hint_text._set					(CStringTable().translate(text));
	}

	m_PartnerCharacterInfo->Show	(!!m_pPartnerInvOwner);
	InitInventoryContents			(m_pInventoryBagList);

	UpdatePocketsPresence();

	TIItemContainer					items_list;
	if ( m_pPartnerInvOwner )
	{
		m_pPartnerInvOwner->inventory().AddAvailableItems( items_list, false, m_pPartnerInvOwner->is_alive() ); //true
		UpdatePartnerBag();
	}
	else
	{
		VERIFY( m_pInvBox );
		m_pInvBox->set_in_use( true );
		m_pInvBox->AddAvailableItems( items_list );
	}

	std::sort( items_list.begin(), items_list.end(),InventoryUtilities::GreaterRoomInRuck );
	
	TIItemContainer::iterator it	= items_list.begin();
	TIItemContainer::iterator it_e	= items_list.end();
	for(; it != it_e; ++it) 
	{
		CUICellItem* itm			= create_cell_item	(*it);
		m_pDeadBodyBagList->SetItem	(itm);
	}

	CBaseMonster* monster = smart_cast<CBaseMonster*>( m_pPartnerInvOwner );
	
	//only for partner, box = no, monster = no
	if (m_pPartnerInvOwner && !monster && !m_pPartnerInvOwner->is_alive())
	{
		CInfoPortionWrapper						known_info_registry;
		known_info_registry.registry().init		(m_pPartnerInvOwner->object_id());
		KNOWN_INFO_VECTOR& known_infos			= known_info_registry.registry().objects();

		KNOWN_INFO_VECTOR_IT it					= known_infos.begin();
		for(int i=0;it!=known_infos.end();++it,++i)
		{
			NET_Packet					P;
			CGameObject::u_EventGen		(P,GE_INFO_TRANSFER, m_pActorInvOwner->object_id());
			P.w_u16						(0);
			P.w_stringZ					(*it);
			P.w_u8						(1);
			CGameObject::u_EventSend	(P);
		}
		known_infos.clear	();
	}
	UpdateDeadBodyBag();
}

void CUIActorMenu::DeInitDeadBodySearchMode()
{
	m_pDeadBodyBagList->Show		(false);
	m_PartnerCharacterInfo->Show	(false);
	m_PartnerWeightInfo->Show		(false);
	m_PartnerWeight->Show			(false);
	m_PartnerVolumeInfo->Show		(false);
	m_PartnerVolume->Show			(false);
	m_takeall_button->Show			(false);

	if (m_pInvBox)
		m_pInvBox->set_in_use		(false);
}

bool CUIActorMenu::ToDeadBodyBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem item = (PIItem)itm->m_pData;
	if (item->IsQuestItem())
		return false;

	if (m_pPartnerInvOwner)
	{
		if (!m_pPartnerInvOwner->deadbody_can_take_status())
			return false;
	}
	else // box
	{
		if (!m_pInvBox->can_take())
			return false;

		luabind::functor<bool> funct;
		if (ai().script_engine().functor("_G.CInventoryBox_CanTake", funct))
			if (funct(m_pInvBox->cast_game_object()->lua_game_object(), item->cast_game_object()->lua_game_object()) == false)
				return false;
	}

	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= NULL;

	if(b_use_cursor_pos)
	{
		new_owner						= CUIDragDropListEx::m_drag_item->BackList();
		VERIFY							(new_owner==m_pDeadBodyBagList);
	}else
		new_owner						= m_pDeadBodyBagList;
	
	CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );

	if(b_use_cursor_pos)
		new_owner->SetItem				(i,old_owner->GetDragItemPosition());
	else
		new_owner->SetItem				(i);

	PIItem iitem						= (PIItem)i->m_pData;

	if ( m_pPartnerInvOwner )
		move_item_from_to				(m_pActorInvOwner->object_id(), m_pPartnerInvOwner->object_id(), iitem->object_id());
	else // box
		move_item_from_to				(m_pActorInvOwner->object_id(), m_pInvBox->ID(), iitem->object_id());
	
	UpdateDeadBodyBag();
	return true;
}

void CUIActorMenu::UpdateDeadBodyBag()
{
	InventoryUtilities::UpdateLabelsValues(m_PartnerWeight, m_PartnerVolume, m_pPartnerInvOwner, m_pInvBox);
	InventoryUtilities::AlighLabels(m_PartnerWeightInfo, m_PartnerWeight, m_PartnerVolumeInfo, m_PartnerVolume);
}

void CUIActorMenu::TakeAllFromPartner(CUIWindow* w, void* d)
{
	if (m_pInvBox)
		TakeAllFromInventoryBox();
}

void CUIActorMenu::TakeAllFromInventoryBox()
{
	luabind::functor<void>funct;
	ai().script_engine().functor("xmod_container_manager.pick_up_box", funct);
	funct(m_pInvBox->lua_game_object());
}