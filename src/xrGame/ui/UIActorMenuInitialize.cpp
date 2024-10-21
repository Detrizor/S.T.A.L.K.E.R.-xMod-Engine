#include "stdafx.h"
#include "UIActorMenu.h"
#include "UIXmlInit.h"
#include "xrUIXmlParser.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UIActorStateInfo.h"
#include "UIItemInfo.h"
#include "UIFrameLineWnd.h"
#include "UIMessageBoxEx.h"
#include "UIPropertiesBox.h"
#include "UI3tButton.h"
#include "UIComboBox.h"

#include "UIInventoryUpgradeWnd.h"
#include "UIInvUpgradeInfo.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "object_broker.h"
#include "UIWndCallback.h"
#include "UIHelper.h"
#include "UIProgressBar.h"
#include "ui_base.h"
#include "../string_table.h"

CUIActorMenu::CUIActorMenu()
{
	m_currMenuMode					= mmUndefined;
	m_trade_partner_inventory_state = 0;
	Construct						();
	currency_str					= *CStringTable().translate("st_currency");
	money_delimiter					= *CStringTable().translate("st_money_delimiter");
}

CUIActorMenu::~CUIActorMenu()
{
	xr_delete			(m_message_box_yes_no);
	xr_delete			(m_message_box_ok);
	xr_delete			(m_UIPropertiesBox);
	xr_delete			(m_hint_wnd);
	xr_delete			(m_ItemInfo);

	ClearAllLists		();
}

void CUIActorMenu::Construct()
{
	CUIXml								uiXml;
	uiXml.Load							(CONFIG_PATH, UI_PATH, "actor_menu.xml");

	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);
	m_hint_wnd = UIHelper::CreateHint	(uiXml, "hint_wnd");

	m_pUpgradeWnd						= xr_new<CUIInventoryUpgradeWnd>(); 
	AttachChild							(m_pUpgradeWnd);
	m_pUpgradeWnd->SetAutoDelete		(true);
	m_pUpgradeWnd->Init					();

	m_ActorCharacterInfo				= xr_new<CUICharacterInfo>();
	m_ActorCharacterInfo->SetAutoDelete	(true);
	AttachChild							(m_ActorCharacterInfo);
	m_ActorCharacterInfo->InitCharacterInfo(&uiXml, "actor_ch_info");

	m_PartnerCharacterInfo				= xr_new<CUICharacterInfo>();
	m_PartnerCharacterInfo->SetAutoDelete(true);
	AttachChild							(m_PartnerCharacterInfo);
	m_PartnerCharacterInfo->InitCharacterInfo( &uiXml, "partner_ch_info" );

	m_pInventoryBagList			= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_bag", this);

	m_pTradeActorBagList		= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_actor_trade_bag", this);
	m_pTradeActorList			= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_actor_trade", this);
	m_pTradePartnerBagList		= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_partner_bag", this);
	m_pTradePartnerList			= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_partner_trade", this);
	m_pDeadBodyBagList			= UIHelper::CreateDragDropListEx(uiXml, "dragdrop_deadbody_bag", this);

	m_ActorInventoryTrade		= UIHelper::CreateStatic(uiXml, "actor_inventory_trade", this);
	m_ActorTradePrice			= UIHelper::CreateTextWnd(uiXml, "actor_inventory_trade:trade_price", m_ActorInventoryTrade);
	m_ActorTradeWeight			= UIHelper::CreateTextWnd(uiXml, "actor_inventory_trade:trade_weight", m_ActorInventoryTrade);
	m_ActorTradeVolume			= UIHelper::CreateTextWnd(uiXml, "actor_inventory_trade:trade_volume", m_ActorInventoryTrade);
	
	m_PartnerInventoryTrade		= UIHelper::CreateStatic(uiXml, "partner_inventory_trade", this);
	m_PartnerTradePrice			= UIHelper::CreateTextWnd(uiXml, "partner_inventory_trade:trade_price", m_PartnerInventoryTrade);
	m_PartnerTradeWeight		= UIHelper::CreateTextWnd(uiXml, "partner_inventory_trade:trade_weight", m_PartnerInventoryTrade);
	m_PartnerTradeVolume		= UIHelper::CreateTextWnd(uiXml, "partner_inventory_trade:trade_volume", m_PartnerInventoryTrade);

	m_pTrashList				= UIHelper::CreateDragDropListEx		(uiXml, "dragdrop_trash", this);
	m_pTrashList->m_f_item_drop	= CUIDragDropListEx::DRAG_CELL_EVENT	(this,&CUIActorMenu::OnItemDrop);
	m_pTrashList->m_f_drag_event= CUIDragDropListEx::DRAG_ITEM_EVENT	(this,&CUIActorMenu::OnDragItemOnTrash);

	m_HelmetOver = UIHelper::CreateStatic(uiXml, "helmet_over", this);
	m_HelmetOver->Show			(false);

	m_ActorMoney	= UIHelper::CreateTextWnd(uiXml, "actor_ch_info:actor_money_static", m_ActorCharacterInfo);
	m_PartnerMoney	= UIHelper::CreateTextWnd(uiXml, "partner_ch_info:partner_money_static", m_PartnerCharacterInfo);

	m_trade_list					= xr_new<CUIComboBox>();
	AttachChild						(m_trade_list);
	m_trade_list->SetAutoDelete		(true);
	CUIXmlInit::InitComboBox		(uiXml, "trade_list", 0, m_trade_list);

	m_trade_buy_button		= UIHelper::Create3tButton(uiXml, "trade_buy_button", this);
	m_trade_sell_button		= UIHelper::Create3tButton(uiXml, "trade_sell_button", this);
	m_takeall_button		= UIHelper::Create3tButton(uiXml, "takeall_button", this);
	m_exit_button			= UIHelper::Create3tButton(uiXml, "exit_button", this);

	//Alun: Dynamic UI slots bro
	XML_NODE* stored_root							= uiXml.GetLocalRoot();
	m_SlotsWnd										= UIHelper::CreateStatic(uiXml, "inventory_slot_wnd", this);
	XML_NODE* node									= uiXml.NavigateToNode("inventory_slot_wnd", 0);
	uiXml.SetLocalRoot								(node);
	m_slot_count									= (u8)uiXml.GetNodesNum(node, "slot") - 1;
	m_pInvList.resize								(m_slot_count + 1);
	m_pInvSlotHighlight.resize						(m_slot_count + 1);
	m_pInvSlotProgress.resize						(m_slot_count + 1);
	for (u8 i = 1; i <= m_slot_count; ++i)
	{
		uiXml.SetLocalRoot							(node);

		m_pInvList[i]								= xr_new<CUIDragDropListEx>();
		m_SlotsWnd->AttachChild						(m_pInvList[i]);
		CUIXmlInit::InitDragDropListEx				(uiXml, "slot", i, m_pInvList[i]);
		m_pInvList[i]->SetAutoDelete				(true);
		BindDragDropListEvents						(m_pInvList[i]);
		
		XML_NODE* slot_node							= uiXml.NavigateToNode("slot", i);
		uiXml.SetLocalRoot							(slot_node);

		m_pInvSlotHighlight[i]						= xr_new<CUIStatic>();
		m_pInvList[i]->AttachChild					(m_pInvSlotHighlight[i]);
		CUIXmlInit::InitStatic						(uiXml, "highlight", 0, m_pInvSlotHighlight[i]);
		m_pInvSlotHighlight[i]->SetAutoDelete		(true);
		m_pInvSlotHighlight[i]->Show				(false);

		if (uiXml.GetNodesNum(slot_node, "slot_progress") == 0)
			continue;

		m_pInvSlotProgress[i]						= xr_new<CUIProgressBar>();
		m_pInvList[i]->AttachChild					(m_pInvSlotProgress[i]);
		CUIXmlInit::InitProgressBar					(uiXml, "slot_progress", 0, m_pInvSlotProgress[i]);
		m_pInvSlotProgress[i]->SetAutoDelete		(true);
	}
	uiXml.SetLocalRoot								(stored_root);
	//-Alun
	
	m_PocketsWnd									= UIHelper::CreateStatic(uiXml, "pockets_wnd", this);
	node											= uiXml.NavigateToNode("pockets_wnd", 0);
	uiXml.SetLocalRoot								(node);
	m_pockets_count									= (u8)uiXml.GetNodesNum(node, "pocket");
	m_pInvPocket.resize								(m_pockets_count);
	m_pPocketInfo.resize							(m_pockets_count);
	m_pPocketOver.resize							(m_pockets_count);
	for (u8 i = 0; i < m_pockets_count; i++)
	{
		uiXml.SetLocalRoot							(node);

		m_pInvPocket[i]								= xr_new<CUIDragDropListEx>();
		m_PocketsWnd->AttachChild					(m_pInvPocket[i]);
		CUIXmlInit::InitDragDropListEx				(uiXml, "pocket", i, m_pInvPocket[i]);
		m_pInvPocket[i]->SetAutoDelete				(true);
		BindDragDropListEvents						(m_pInvPocket[i]);
		m_pInvPocket[i]->m_f_item_drop				= CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemDrop);
		m_pInvPocket[i]->m_f_drag_event				= CUIDragDropListEx::DRAG_ITEM_EVENT(this, &CUIActorMenu::OnDragItemOnPocket);
		
		XML_NODE* pocket_node						= uiXml.NavigateToNode("pocket", i);
		uiXml.SetLocalRoot							(pocket_node);

		m_pPocketInfo[i]							= UIHelper::CreateTextWnd(uiXml, "info", m_pInvPocket[i]);
		m_pPocketOver[i]							= UIHelper::CreateStatic(uiXml, "over", m_pInvPocket[i]);
		m_pPocketOver[i]->Show						(false);
	}
	uiXml.SetLocalRoot								(stored_root);

	m_ActorWeightInfo			= UIHelper::CreateStatic(uiXml, "actor_weight_caption", this);
	m_ActorWeight				= UIHelper::CreateTextWnd(uiXml, "actor_weight", this);
	m_ActorWeightInfo->AdjustWidthToText();

	m_ActorVolumeInfo			= UIHelper::CreateStatic(uiXml, "actor_volume_caption", this);
	m_ActorVolume				= UIHelper::CreateTextWnd(uiXml, "actor_volume", this);
	m_ActorVolumeInfo->AdjustWidthToText();

	m_PartnerWeightInfo			= UIHelper::CreateStatic(uiXml, "partner_weight_caption", this);
	m_PartnerWeight				= UIHelper::CreateTextWnd(uiXml, "partner_weight", this);
	m_PartnerWeightInfo->AdjustWidthToText();

	m_PartnerVolumeInfo			= UIHelper::CreateStatic(uiXml, "partner_volume_caption", this);
	m_PartnerVolume				= UIHelper::CreateTextWnd(uiXml, "partner_volume", this);
	m_PartnerVolumeInfo->AdjustWidthToText();

	stored_root							= uiXml.GetLocalRoot();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode	("action_sounds",0));
	::Sound->create						(sounds[eItemToSlot],	uiXml.Read("snd_item_to_slot",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eItemToHands],	uiXml.Read("snd_item_to_hands",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eItemToRuck],	uiXml.Read("snd_item_to_ruck",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eProperties],	uiXml.Read("snd_properties",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eAttachAddon],	uiXml.Read("snd_attach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eDetachAddon],	uiXml.Read("snd_detach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eItemUse],		uiXml.Read("snd_item_use",		0,	NULL),st_Effect,sg_SourceType);
	uiXml.SetLocalRoot					(stored_root);

	m_ItemInfo							= xr_new<CUIItemInfo>();
	m_ItemInfo->InitItemInfo			("actor_menu_item.xml");

	m_upgrade_info						= NULL;
	if ( ai().get_alife() )
	{
		m_upgrade_info						= xr_new<UIInvUpgradeInfo>();
		m_upgrade_info->init_from_xml		("actor_menu_item.xml");
	}

	m_message_box_yes_no				= xr_new<CUIMessageBoxEx>();	
	m_message_box_yes_no->InitMessageBox( "message_box_yes_no" );
	m_message_box_yes_no->SetAutoDelete	(true);
	m_message_box_yes_no->SetText		( "" );

	m_message_box_ok					= xr_new<CUIMessageBoxEx>();	
	m_message_box_ok->InitMessageBox	( "message_box_ok" );
	m_message_box_ok->SetAutoDelete		(true);
	m_message_box_ok->SetText			( "" );

	m_UIPropertiesBox					= xr_new<CUIPropertiesBox>();
	m_UIPropertiesBox->InitPropertiesBox(Fvector2().set(0,0),Fvector2().set(300,300));
	AttachChild							(m_UIPropertiesBox);
	m_UIPropertiesBox->Hide				();
	m_UIPropertiesBox->SetWindowName	( "property_box" );

	InitCallbacks						();
	
	BindDragDropListEvents				(m_pInventoryBagList);
	BindDragDropListEvents				(m_pTradeActorBagList);
	BindDragDropListEvents				(m_pTradeActorList);
	BindDragDropListEvents				(m_pTradePartnerBagList);
	BindDragDropListEvents				(m_pTradePartnerList);
	BindDragDropListEvents				(m_pDeadBodyBagList);
	BindDragDropListEvents				(m_pTrashList);

	m_allowed_drops[iTrashSlot].push_back(iActorSlot);
	m_allowed_drops[iTrashSlot].push_back(iActorPocket);
	m_allowed_drops[iTrashSlot].push_back(iActorBag);
	m_allowed_drops[iTrashSlot].push_back(iDeadBodyBag);

	m_allowed_drops[iActorSlot].push_back(iActorSlot);
	m_allowed_drops[iActorSlot].push_back(iActorPocket);
	m_allowed_drops[iActorSlot].push_back(iActorBag);
	m_allowed_drops[iActorSlot].push_back(iTrashSlot);
	m_allowed_drops[iActorSlot].push_back(iActorTrade);
	m_allowed_drops[iActorSlot].push_back(iDeadBodyBag);

	m_allowed_drops[iActorBag].push_back(iActorSlot);
	m_allowed_drops[iActorBag].push_back(iActorPocket);
	m_allowed_drops[iActorBag].push_back(iActorBag);
	m_allowed_drops[iActorBag].push_back(iTrashSlot);
	m_allowed_drops[iActorBag].push_back(iActorTrade);
	m_allowed_drops[iActorBag].push_back(iDeadBodyBag);

	m_allowed_drops[iActorPocket].push_back(iActorSlot);
	m_allowed_drops[iActorPocket].push_back(iActorPocket);
	m_allowed_drops[iActorPocket].push_back(iActorBag);
	m_allowed_drops[iActorPocket].push_back(iTrashSlot);
	m_allowed_drops[iActorPocket].push_back(iActorTrade);
	m_allowed_drops[iActorPocket].push_back(iDeadBodyBag);

	m_allowed_drops[iActorTrade].push_back(iActorSlot);
	m_allowed_drops[iActorTrade].push_back(iActorPocket);
	m_allowed_drops[iActorTrade].push_back(iActorBag);
	m_allowed_drops[iActorTrade].push_back(iActorTrade);

	m_allowed_drops[iPartnerTrade].push_back(iPartnerTrade);
	m_allowed_drops[iPartnerTrade].push_back(iPartnerTradeBag);

	m_allowed_drops[iPartnerTradeBag].push_back(iPartnerTrade);

	m_allowed_drops[iDeadBodyBag].push_back(iActorSlot);
	m_allowed_drops[iDeadBodyBag].push_back(iActorPocket);
	m_allowed_drops[iDeadBodyBag].push_back(iActorBag);
	m_allowed_drops[iDeadBodyBag].push_back(iTrashSlot);
	m_allowed_drops[iDeadBodyBag].push_back(iDeadBodyBag);

	m_upgrade_selected				= nullptr;
	InfoCurItem						(nullptr);
	SetCurrentItem					(nullptr);
	SetActor						(nullptr);
	SetPartner						(nullptr);
	SetInvBox						(nullptr);
	SetContainer					(nullptr);
	SetBag							(nullptr);

	m_actor_trade					= nullptr;
	m_partner_trade					= nullptr;
	m_repair_mode					= false;
	m_item_info_view				= false;
	m_highlight_clear				= true;

	DeInitInventoryMode				();
	DeInitTradeMode					();
	DeInitUpgradeMode				();
	DeInitDeadBodySearchMode		();
}

void CUIActorMenu::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemRButtonClick);
	lst->m_f_item_focus_received	= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemFocusReceive);
	lst->m_f_item_focus_lost		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemFocusLost);
	lst->m_f_item_focused_update	= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIActorMenu::OnItemFocusedUpdate);
}

void CUIActorMenu::InitCallbacks()
{
	Register						(m_trade_list);
	Register						(m_trade_buy_button);
	Register						(m_trade_sell_button);
	Register						(m_takeall_button);
	Register						(m_exit_button);
	Register						(m_UIPropertiesBox);
	VERIFY							(m_pUpgradeWnd);
	Register						(m_pUpgradeWnd->m_btn_repair);

	AddCallback(m_trade_list,					LIST_ITEM_SELECT,	CUIWndCallback::void_function(this, &CUIActorMenu::OnTradeList));
	AddCallback(m_trade_buy_button,				BUTTON_CLICKED,		CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeBuy));
	AddCallback(m_trade_sell_button,			BUTTON_CLICKED,		CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeSell));
	AddCallback(m_takeall_button,				BUTTON_CLICKED,		CUIWndCallback::void_function(this, &CUIActorMenu::TakeAllFromPartner));
	AddCallback(m_exit_button,					BUTTON_CLICKED,		CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnExitClicked));
	AddCallback(m_UIPropertiesBox,				PROPERTY_CLICKED,	CUIWndCallback::void_function(this, &CUIActorMenu::ProcessPropertiesBoxClicked));
	AddCallback(m_pUpgradeWnd->m_btn_repair,	BUTTON_CLICKED,		CUIWndCallback::void_function(this, &CUIActorMenu::TryRepairItem));
}
