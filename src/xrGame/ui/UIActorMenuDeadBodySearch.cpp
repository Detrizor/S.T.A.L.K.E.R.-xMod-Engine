#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIDragDropListEx.h"
#include "UICharacterInfo.h"
#include "UIInventoryUtilities.h"
#include "UI3tButton.h"
#include "UICellItem.h"
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
#include "../item_container.h"

// -------------------------------------------------------------------------------------------------

void CUIActorMenu::InitDeadBodySearchMode()
{
	m_pDeadBodyBagList->Show		(true);
	m_PartnerWeightInfo->Show		(true);
	m_PartnerWeight->Show			(true);
	m_PartnerVolumeInfo->Show		(true);
	m_PartnerVolume->Show			(true);

	if (m_pInvBox || m_pContainer)
	{
		m_takeall_button->Show								(true);
		m_takeall_button->Enable							(true);
		shared_str text										= (m_pContainer) ? "st_pick_up" : "st_close";
		m_takeall_button->TextItemControl()->SetText		(*CStringTable().translate(text));
		text.printf											("%s_hint", *text);
		m_takeall_button->m_hint_text._set					(CStringTable().translate(text));
		if (m_pInvBox)
			m_pInvBox->set_in_use							(true);
	}

	m_PartnerCharacterInfo->Show		(!!m_pPartnerInvOwner);
	InitInventoryContents				();
	UpdatePocketsPresence				();
	init_dead_body_bag					();

	CBaseMonster* monster				= smart_cast<CBaseMonster*>(m_pPartnerInvOwner);
	
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
}

void CUIActorMenu::init_dead_body_bag()
{
	TIItemContainer						items_list;
	if (m_pPartnerInvOwner)
	{
		m_pPartnerInvOwner->inventory().AddAvailableItems(items_list, false, m_pPartnerInvOwner->is_alive());
		UpdatePartnerBag				();
	}
	else if (m_pInvBox)
		m_pInvBox->m_pContainer->AddAvailableItems(items_list);
	else
		m_pContainer->AddAvailableItems	(items_list);

	_STD sort							(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);
	for (auto& item : items_list)
		m_pDeadBodyBagList->SetItem		(item->getIcon());
	
	InventoryUtilities::UpdateLabelsValues(m_PartnerWeight, m_PartnerVolume, m_pPartnerInvOwner, (m_pInvBox) ? m_pInvBox->m_pContainer : m_pContainer);
	//InventoryUtilities::AlighLabels(m_PartnerWeightInfo, m_PartnerWeight, m_PartnerVolumeInfo, m_PartnerVolume);

	m_pDeadBodyBagList->setValid		(true);
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
	PIItem item						= static_cast<PIItem>(itm->m_pData);
	if (m_pPartnerInvOwner)
	{
		if (!m_pPartnerInvOwner->deadbody_can_take_status())
			return					false;
	}
	else if (m_pInvBox)
	{
		if (!m_pInvBox->can_take() || !m_pInvBox->m_pContainer->CanTakeItem(item))
			return					false;

		::luabind::functor<bool>	funct;
		if (ai().script_engine().functor("_G.CInventoryBox_CanTake", funct))
			if (funct(m_pInvBox->cast_game_object()->lua_game_object(), item->cast_game_object()->lua_game_object()) == false)
				return				false;
	}
	else if (!m_pContainer->CanTakeItem(item))
		return						false;

	item->O.transfer				((m_pPartnerInvOwner) ? m_pPartnerInvOwner->object_id() : ((m_pInvBox) ? m_pInvBox->ID() : m_pContainer->O.ID()));
	
	return							true;
}

void CUIActorMenu::TakeAllFromPartner(CUIWindow* w, void* d)
{
	if (m_pInvBox || m_pContainer)
		TakeAllFromInventoryBox();
}

void CUIActorMenu::TakeAllFromInventoryBox()
{
	if (m_pContainer)
		m_pContainer->O.transfer	(m_pActorInvOwner->object_id());
	HideDialog						();
}
