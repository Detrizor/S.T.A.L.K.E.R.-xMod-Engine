#include "stdafx.h"
#include "UIMpTradeWnd.h"

#include "UIXmlInit.h"
#include "UIMpItemsStoreWnd.h"
#include "UITabControl.h"
#include "UITabButtonMP.h"
#include "UIDragDropListEx.h"
#include "UIItemInfo.h"
#include "UIHelper.h"
#include "UIBuyWeaponTab.h"

#include "object_broker.h"


LPCSTR _list_names[]= {
		"lst_pistol",
		"lst_pistol_ammo",
		"lst_rifle",
		"lst_rifle_ammo",
		"lst_outfit",
		"lst_medkit",
		"lst_granade",
		"lst_others",
		"lst_player_bag",
		"lst_shop",
};
CUIMpTradeWnd::CUIMpTradeWnd()
{
	m_money								= 0;
	g_mp_restrictions.InitGroups		();
	m_bIgnoreMoneyAndRank				= false;
}

CUIMpTradeWnd::~CUIMpTradeWnd()
{
	m_root_tab_control->RemoveAll		();
	delete_data							(m_store_hierarchy);
	delete_data							(m_list[e_shop]);
	delete_data							(m_all_items);
	delete_data							(m_item_mngr);
}

void CUIMpTradeWnd::Init(const shared_str& sectionName, const shared_str& sectionPrice)
{
}
