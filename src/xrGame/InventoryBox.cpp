#include "pch_script.h"
#include "InventoryBox.h"
#include "script_game_object.h"

CInventoryBox::CInventoryBox()
{
	m_in_use							= false;
	m_can_take							= true;
	m_closed							= false;
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
	m_pContainer						= AddModule<CInventoryContainer>();
	return								res;
}

void CInventoryBox::SE_update_status()
{
	NET_Packet* P						= xr_new<NET_Packet>();
	CGameObject::u_EventGen				( *P, GE_INV_BOX_STATUS, ID() );
	P->w_u8								( (m_can_take)? 1 : 0 );
	P->w_u8								( (m_closed)? 1 : 0 );
	P->w_stringZ							( tip_text() );
	CGameObject::u_EventSend			( *P );
	xr_delete							(P);
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

float CInventoryBox::Aboba o$(EEventTypes type, void* data, int param)
{
	if (type == eOnChild && !param && m_in_use)
	{
		CGameObject* GO					= Cast<CGameObject*>((CObject*)data);
		Actor()->callback				(GameObject::eInvBoxItemTake)(lua_game_object(), GO->lua_game_object());
	}

	return								inherited::Aboba(type, data, param);
}
