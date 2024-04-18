#pragma once

#include "script_export_space.h"

#include "UIDialogWnd.h"
#include "UIWndCallback.h"
#include "../../xrServerEntities/inventory_space.h"
#include "UIHint.h"

#include "script_game_object.h" //Alundaio

class CUICharacterInfo;
class CUIDragDropListEx;
class CUIDragDropReferenceList;
class CUICellItem;
class CUIDragItem;
class CUIItemInfo;
class CUIFrameLineWnd;
class CUIStatic;
class CUITextWnd;
class CUI3tButton;
class CUIComboBox;
class CInventoryOwner;
class CInventoryBox;
class CUIInventoryUpgradeWnd;
class UIInvUpgradeInfo;
class CUIMessageBoxEx;
class CUIPropertiesBox;
class CTrade;
class CUIProgressBar;
class CInventoryContainer;

namespace inventory { namespace upgrade {
	class Upgrade;
} } // namespace upgrade, inventory

enum EDDListType{
		iInvalid,
		iActorSlot,
		iActorBag,
		iActorPocket,

		iActorTrade,
		iPartnerTradeBag,
		iPartnerTrade,
		iDeadBodyBag,
		iTrashSlot,
		iListTypeMax
};

enum EMenuMode{
		mmUndefined,
		mmInventory,
		mmTrade,
		mmUpgrade,
		mmDeadBodySearch,
		mmInvBoxSearch,
		mmContainerSearch,
};

enum eActorMenuSndAction{
	eItemToSlot = 0,
	eItemToHands,
	eItemToRuck,
	eProperties,
	eAttachAddon,
	eDetachAddon,
	eItemUse,
	eSndMax
};

class CUIActorMenu :	public CUIDialogWnd, 
						public CUIWndCallback
{
	typedef CUIDialogWnd		inherited;
	typedef inventory::upgrade::Upgrade 	Upgrade_type;

protected:
	EMenuMode					m_currMenuMode;
	ref_sound					sounds					[eSndMax];

	UIHint*						m_hint_wnd;
	CUIItemInfo*				m_ItemInfo;
	CUICellItem*				m_InfoCellItem;
	u32							m_InfoCellItem_timer;
	CUICellItem*				m_pCurrentCellItem;

	CUICellItem*				m_upgrade_selected;
	CUIPropertiesBox*			m_UIPropertiesBox;

	CUICharacterInfo*			m_ActorCharacterInfo;
	CUICharacterInfo*			m_PartnerCharacterInfo;

	CUIDragDropListEx*					m_pInventoryBagList;

	u8									m_pockets_count;
	CUIStatic*							m_PocketsWnd;
	xr_vector<CUIDragDropListEx*>		m_pInvPocket;
	xr_vector<CUITextWnd*>				m_pPocketInfo;
	xr_vector<CUIStatic*>				m_pPocketOver;

	CUIDragDropListEx*			m_pTradeActorBagList;
	CUIDragDropListEx*			m_pTradeActorList;
	CUIDragDropListEx*			m_pTradePartnerBagList;
	CUIDragDropListEx*			m_pTradePartnerList;
	CUIDragDropListEx*			m_pDeadBodyBagList;
	
	CUIStatic*					m_HelmetOver;

	u8									m_slot_count;
	CUIStatic*							m_SlotsWnd;
	xr_vector<CUIDragDropListEx*>		m_pInvList;
	xr_vector<CUIStatic*>				m_pInvSlotHighlight;
	xr_vector<CUIProgressBar*>			m_pInvSlotProgress;

	CUIInventoryUpgradeWnd*		m_pUpgradeWnd;

	UIInvUpgradeInfo*			m_upgrade_info;
	CUIMessageBoxEx*			m_message_box_yes_no;
	CUIMessageBoxEx*			m_message_box_ok;

	CInventoryOwner*			m_pActorInvOwner;
	CInventory*					m_pActorInv;
	CInventoryOwner*			m_pPartnerInvOwner;
	CInventoryBox*				m_pInvBox;
	CInventoryContainer*		m_pBag;
	CInventoryContainer*		m_pContainer;

	CUITextWnd*					m_ActorMoney;
	CUITextWnd*					m_PartnerMoney;
	
	// bottom ---------------------------------
	CUIStatic*					m_ActorWeightInfo;
	CUITextWnd*					m_ActorWeight;
	CUIStatic*					m_ActorVolumeInfo;
	CUITextWnd*					m_ActorVolume;
	
	CUIStatic*					m_PartnerWeightInfo;
	CUITextWnd*					m_PartnerWeight;
	CUIStatic*					m_PartnerVolumeInfo;
	CUITextWnd*					m_PartnerVolume;

	// delimiter ------------------------------
	CUIStatic*					m_PartnerInventoryTrade;
	CUITextWnd*					m_PartnerTradePrice;
	CUITextWnd*					m_PartnerTradeWeight;
	CUITextWnd*					m_PartnerTradeVolume;

	CUIStatic*					m_ActorInventoryTrade;
	CUITextWnd*					m_ActorTradePrice;
	CUITextWnd*					m_ActorTradeWeight;
	CUITextWnd*					m_ActorTradeVolume;

	CTrade*						m_actor_trade;
	CTrade*						m_partner_trade;

	CUIComboBox*				m_trade_list;

	CUI3tButton*				m_trade_buy_button;
	CUI3tButton*				m_trade_sell_button;
	CUI3tButton*				m_takeall_button;
	CUI3tButton*				m_exit_button;

	u32							m_last_time;
	bool						m_repair_mode;
	bool						m_item_info_view;
	bool						m_highlight_clear;
	u32							m_trade_partner_inventory_state;
	LPCSTR						currency_str;
	LPCSTR						money_delimiter;

public:
	CUIDragDropListEx*							m_pTrashList;

public:
	void						StartMenuMode				(EMenuMode mode, CInventoryOwner* actor, void* partner = NULL);
	void						SetMenuMode					(EMenuMode mode, void* partner = NULL, bool forced = false);
	EMenuMode					GetMenuMode					() {return m_currMenuMode;};

	void						SetActor					(CInventoryOwner* io);
	void						SetPartner					(CInventoryOwner* io);
	void						SetInvBox					(CInventoryBox* box);
	void						SetContainer				(CInventoryContainer* ciitem);
	void						SetBag						(CInventoryContainer* bag_cont);

	CInventoryOwner*			GetPartner					() {return m_pPartnerInvOwner;};
	CInventoryBox*				GetInvBox					() {return m_pInvBox;};
	CInventoryContainer*		GetContainer				() {return m_pContainer;};
	CInventoryContainer*		GetBag						() {return m_pBag;};
	
	void						PlaySnd						(eActorMenuSndAction a);
	LPCSTR						FormatMoney					(u32 money);

private:
	void						PropertiesBoxForSlots		(PIItem item, bool& b_show);
	void						PropertiesBoxForAddonOwner	(PIItem item, bool& b_show);
	void						PropertiesBoxForAddon		(PIItem item, bool& b_show);
	void						PropertiesBoxForUsing		(PIItem item, bool& b_show);
	void						PropertiesBoxForPlaying		(PIItem item, bool& b_show);
	void						PropertiesBoxForDrop		(CUICellItem* cell_item, PIItem item, bool& b_show);
	void						PropertiesBoxForRepair		(PIItem item, bool& b_show);
	void						PropertiesBoxForDonate		(PIItem item, bool& b_show); //Alundaio

private:
	void						clear_highlight_lists		();
	void						set_highlight_item			(CUICellItem* cell_item);
	void						highlight_item_slot			(CUICellItem* cell_item);
	void						highlight_armament			(CUICellItem* cell_item, CUIDragDropListEx* ddlist);

protected:			
	void						Construct					();
	void						InitCallbacks				();

	void						InitCellForSlot				(u16 slot_idx);
	void						InitPocket					(u16 pocket_idx);
	void						ClearAllLists				();
	void						BindDragDropListEvents		(CUIDragDropListEx* lst);
	
	EDDListType					GetListType					(CUIDragDropListEx* l);
	CUIDragDropListEx*			GetListByType				(EDDListType t);
	CUIDragDropListEx*			GetSlotList					(u16 slot_idx);
	u16							GetPocketIdx				(CUIDragDropListEx* pocket);
	bool						CanSetItemToList			(PIItem item, CUIDragDropListEx* l, u16& ret_slot);
	
	xr_vector<EDDListType>		m_allowed_drops				[iListTypeMax];
	bool						AllowItemDrops				(EDDListType from, EDDListType to);

	bool		xr_stdcall		OnItemDrop					(CUICellItem* itm);
	bool		xr_stdcall		OnItemStartDrag				(CUICellItem* itm);
	bool		xr_stdcall		OnItemDbClick				(CUICellItem* itm);
	bool		xr_stdcall		OnItemSelected				(CUICellItem* itm);
	bool		xr_stdcall		OnItemRButtonClick			(CUICellItem* itm);
	bool		xr_stdcall		OnItemFocusReceive			(CUICellItem* itm);
	bool		xr_stdcall		OnItemFocusLost				(CUICellItem* itm);
	bool		xr_stdcall		OnItemFocusedUpdate			(CUICellItem* itm);
	void		xr_stdcall		OnDragItemOnTrash			(CUIDragItem* item, bool b_receive);
	void		xr_stdcall		OnDragItemOnPocket			(CUIDragItem* item, bool b_receive);

	void						ResetMode					();
	void						InitInventoryMode			();
	void						DeInitInventoryMode			();
	void						InitTradeMode				();
	void						DeInitTradeMode				();
	void						InitUpgradeMode				();
	void						DeInitUpgradeMode			();
	void						InitDeadBodySearchMode		();
	void						DeInitDeadBodySearchMode	();

	void						CurModeToScript				();
	void						RepairEffect_CurItem		();

	//void						SetCurrentItem				(CUICellItem* itm); //Alundaio: Made public
	//CUICellItem*				CurrentItem					();					//Alundaio: Made public
	PIItem						CurrentIItem				();

	void						InfoCurItem					(CUICellItem* cell_item); //on update item

	void						ActivatePropertiesBox		();
	void						TryHidePropertiesBox		();
	void		xr_stdcall		ProcessPropertiesBoxClicked	(CUIWindow* w, void* d);
	
	void						CheckDistance				();
	void						UpdateItemsPlace			();

	void						SetupUpgradeItem			();
	void						UpdateUpgradeItem			();
	void						TrySetCurUpgrade			();
	void						UpdateButtonsLayout			();

	// inventory
	bool						ToSlot						(CUICellItem* itm, u16 slot_id, bool assume_alternative = false);
	bool						ToPocket					(CUICellItem* itm, bool b_use_cursor_pos, u16 pocket_id);
	bool						ToBag						(CUICellItem* itm, bool b_use_cursor_pos);
	void						ToRuck						(PIItem item);
	bool						TryUseItem					(CUICellItem* cell_itm);

	void						UpdateOutfit				();
	void		xr_stdcall		TryRepairItem				(CUIWindow* w, void* d);
	bool						CanUpgradeItem				(PIItem item);

	bool						ToActorTrade				(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToPartnerTrade				(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToPartnerTradeBag			(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToDeadBodyBag				(CUICellItem* itm, bool b_use_cursor_pos);

	void						detach_addon				(CAddon* addon);
	
	void						SendEvent_PickUpItem		(PIItem	pItem, u16 place = eItemPlaceUndefined, u16 idx = 0);
	void						SendEvent_Item_Eat			(PIItem	pItem, u16 parent);
	void						DropAllCurrentItem			();
	void						OnPressUserKey				();

	// trade
	void						InitPartnerInventoryContents();
	void						ColorizeItem				(CUICellItem* itm);
	float						CalcItemsWeight				(CUIDragDropListEx* pList);
	float						CalcItemsVolume				(CUIDragDropListEx* pList);
	u32							CalcItemsPrice				(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying);
	void						UpdatePrices				();
	bool						CanMoveToPartner			(PIItem pItem, shared_str* reason = NULL);
	void						TransferItems				(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying);

public:
								CUIActorMenu				();
	virtual						~CUIActorMenu				();

	virtual bool				StopAnyMove					();
	virtual void				SendMessage					(CUIWindow* pWnd, s16 msg, void* pData = NULL);
	virtual void				Draw						();
	virtual void				Update						();
	virtual void				Show						(bool status);

	virtual bool				OnKeyboardAction					(int dik, EUIMessages keyboard_action);
	virtual bool				OnMouseAction						(float x, float y, EUIMessages mouse_action);

	void						CallMessageBoxYesNo			(LPCSTR text);
	void						CallMessageBoxOK			(LPCSTR text);
	void		xr_stdcall		OnMesBoxYes					(CUIWindow*, void*);
	void		xr_stdcall		OnMesBoxNo					(CUIWindow*, void*);

	void						OnInventoryAction			(PIItem pItem, bool take, u8 zone);
	void						ShowRepairButton			(bool status);
	bool						SetInfoCurUpgrade			(Upgrade_type* upgrade_type, CInventoryItem* inv_item );
	void						SeparateUpgradeItem			();
	PIItem						get_upgrade_item			();
	bool						DropAllItemsFromRuck		(bool quest_force = false); //debug func

	void						UpdateActor					();
	void						UpdatePartnerBag			();
	void						UpdateDeadBodyBag			();
	
	void		xr_stdcall		OnTradeList					(CUIWindow* w, void* d);
	void		xr_stdcall		OnBtnPerformTradeBuy		(CUIWindow* w, void* d);
	void		xr_stdcall		OnBtnPerformTradeSell		(CUIWindow* w, void* d);
	void		xr_stdcall		OnBtnExitClicked			(CUIWindow* w, void* d);
	void		xr_stdcall		TakeAllFromPartner			(CUIWindow* w, void* d);
	void						TakeAllFromInventoryBox		();
	void						UpdateConditionProgressBars	();

	void						UpdatePocketsPresence		();
	void						ToggleBag					(CInventoryContainer* bag);

	IC	UIHint*					get_hint_wnd				() { return m_hint_wnd; }

			float			CalcItemWeight			(LPCSTR section);
			float			CalcItemVolume			(LPCSTR section);
			
	void						InitInventoryContents		();

	//AxelDominator && Alundaio consumable use condition
	void RefreshCurrentItemCell();
	void SetCurrentItem(CUICellItem* itm);		//Alundaio: Made public
	CUICellItem* CurrentItem();					//Alundaio: Made public

	CScriptGameObject* GetCurrentItemAsGameObject();
	void HighlightSectionInSlot(LPCSTR section, u8 type, u16 slot_id = 0);
	void HighlightForEachInSlot(const luabind::functor<bool> &functor, u8 type, u16 slot_id);

	//-AxelDominator && Alundaio consumable use condition
	void DonateCurrentItem(CUICellItem* cell_item); //Alundaio: Donate item via context menu while in trade menu
	DECLARE_SCRIPT_REGISTER_FUNCTION

public:
	bool								AttachAddon								(CAddonOwner* ao, CAddon* addon, CAddonSlot* slot = NULL);
}; // class CUIActorMenu

add_to_type_list(CUIActorMenu)
#undef script_type_list
#define script_type_list save_type_list(CUIActorMenu)
