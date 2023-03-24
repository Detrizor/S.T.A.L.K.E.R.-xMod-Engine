#include "pch_script.h"
#include "InventoryBox.h"
#include "level.h"
#include "actor.h"
#include "game_object_space.h"

#include "script_callback_ex.h"
#include "script_game_object.h"
#include "ui/UIActorMenu.h"
#include "uigamecustom.h"
#include "inventory_item.h"
#include "IItemContainer.h"

CInventoryStorage::CInventoryStorage()
{
	m_items.clear						();
}

DLL_Pure* CInventoryStorage::_construct()
{
	m_object							= smart_cast<CGameObject*>(this);
	return								m_object;
}

void CInventoryStorage::OnEvent(NET_Packet& P, u16 type)
{
	u16 id								= NO_ID;
	CObject* obj						= NULL;
	bool dont_create_shell				= false;

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
        P.r_u16							(id);
		obj								= Level().Objects.net_Find(id);
		dont_create_shell				= (type == GE_TRADE_SELL) || (!P.r_eof() && P.r_u8());
	}

	OnEventImpl							(type, id, obj, dont_create_shell);
}

void CInventoryStorage::OnEventImpl(u16 type, u16 id, CObject* obj, bool dont_create_shell)
{
	PIItem item							= smart_cast<PIItem>(obj);
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		m_items.push_back				(item);
		obj->H_SetParent				(m_object);
		obj->setVisible					(FALSE);
		obj->setEnabled					(FALSE);
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		m_items.erase					(std::find(m_items.begin(), m_items.end(), item));
		obj->H_SetParent				(NULL, dont_create_shell);
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CInventoryContainer::CInventoryContainer()
{
	m_capacity							= 0.f;
}

void CInventoryContainer::Load(LPCSTR section)
{
	m_capacity							= pSettings->r_float(section, "capacity");
}

void CInventoryContainer::OnInventoryAction(PIItem item, u16 actionType) const
{
	CUIActorMenu* actor_menu				= (CurrentGameUI()) ? &CurrentGameUI()->GetActorMenu() : NULL;
	if (actor_menu && actor_menu->IsShown())
	{
		u8 res_zone							= 0;
		if (smart_cast<CInventoryContainer*>(actor_menu->GetBag()) == this)
			res_zone						= 3;
		else if (!m_object->H_Parent())
			res_zone						= 2;

		if (res_zone > 0)
			actor_menu->OnInventoryAction	(item, actionType, res_zone);
	}
}

void CInventoryContainer::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	inherited::OnEventImpl				(type, id, itm, dont_create_shell);
	PIItem item							= smart_cast<PIItem>(itm);
	OnInventoryAction					(item, type);
}

bool CInventoryContainer::CanTakeItem(PIItem item) const
{
	if (smart_cast<CInventoryContainer*>(item) == this)
		return							false;
	if (fEqual(m_capacity, 0.f))
		return							true;
	float vol							= ItemsVolume();
	return								(fLessOrEqual(vol, m_capacity) && fLessOrEqual(vol + item->Volume(), m_capacity + 0.1f));
}

void CInventoryContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_items)
		items_container.push_back		(I);
}

float CInventoryContainer::ItemsWeight() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Weight();
	return								res;
}

float CInventoryContainer::ItemsVolume() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Volume();
	return								res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CInventoryBox::CInventoryBox()
{
	m_in_use							= false;
	m_can_take							= true;
	m_closed							= false;
}

DLL_Pure* CInventoryBox::_construct()
{
	inherited::_construct				();
	CInventoryContainer::_construct		();
	return								this;
}

void CInventoryBox::Load(LPCSTR section)
{
	inherited::Load						(section);
	CInventoryContainer::Load			(section);
}

void CInventoryBox::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent					(P, type);
	CInventoryContainer::OnEvent		(P, type);
}

#include "../xrServerEntities/xrServer_Objects_Alife.h"
BOOL CInventoryBox::net_Spawn(CSE_Abstract* DC)
{
	BOOL res							= inherited::net_Spawn(DC);
	setVisible							(TRUE);
	setEnabled							(TRUE);
	set_tip_text						("inventory_box_use");
	CSE_ALifeInventoryBox* pSE_box	= smart_cast<CSE_ALifeInventoryBox*>(DC);
	if (pSE_box)
	{
		m_can_take						= pSE_box->m_can_take;
		m_closed						= pSE_box->m_closed;
		set_tip_text					(pSE_box->m_tip_text.c_str());
	}
	return								res;
}

void CInventoryBox::SE_update_status()
{
	NET_Packet							P;
	CGameObject::u_EventGen				( P, GE_INV_BOX_STATUS, ID() );
	P.w_u8								( (m_can_take)? 1 : 0 );
	P.w_u8								( (m_closed)? 1 : 0 );
	P.w_stringZ							( tip_text() );
	CGameObject::u_EventSend			( P );
}

void CInventoryBox::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	CInventoryContainer::OnEventImpl	(type, id, itm, dont_create_shell);
	
	switch (type)
	{
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		if (m_in_use)
		{
			CGameObject* GO				= smart_cast<CGameObject*>(itm);
			Actor()->callback			(GameObject::eInvBoxItemTake)(lua_game_object(), GO->lua_game_object());
		}
		break;
	}
}

void CInventoryBox::set_can_take(bool status)
{
	m_can_take							= status;
	SE_update_status					();
}

void CInventoryBox::set_closed(bool status, LPCSTR reason)
{
	m_closed							= status;
	if (reason && xr_strlen(reason))
		set_tip_text					(reason);
	else
		set_tip_text					("inventory_box_use");
	SE_update_status					();
}