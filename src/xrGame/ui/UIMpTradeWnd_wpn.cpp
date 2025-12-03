#include "stdafx.h"
#include "UIMpTradeWnd.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "../weaponmagazinedwgrenade.h"
#include "../../xrEngine/xr_input.h"
#include "UIMpItemsStoreWnd.h"

void CUIMpTradeWnd::OnBtnPistolAmmoClicked(CUIWindow* w, void* d)
{
}

void CUIMpTradeWnd::OnBtnPistolSilencerClicked(CUIWindow* w, void* d)
{
	CheckDragItemToDestroy				();
	CUIDragDropListEx*	res			= m_list[e_pistol];
	CUICellItem* ci					= (res->ItemsCount())?res->GetItemIdx(0):NULL;
	if(!ci)	
		return;

	SBuyItemInfo* pitem				= FindItem(ci);
	if(IsAddonAttached(pitem, at_silencer))
	{//detach
		SellItemAddons				(pitem,at_silencer);
	}else
	if(CanAttachAddon(pitem, at_silencer))
	{//attach
		shared_str addon_name		= GetAddonNameSect(pitem,at_silencer);

		if ( NULL==m_store_hierarchy->FindItem(addon_name) )
			return;
						
		SBuyItemInfo* addon_item	= CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
		bool b_res_addon			= TryToBuyItem(addon_item, bf_normal, pitem );
		if(!b_res_addon)
			DestroyItem				(addon_item);
	}
}

void CUIMpTradeWnd::OnBtnRifleAmmoClicked(CUIWindow* w, void* d)
{
}

void CUIMpTradeWnd::OnBtnRifleSilencerClicked(CUIWindow* w, void* d)
{
	CheckDragItemToDestroy				();
	CUIDragDropListEx*	res			= m_list[e_rifle];
	CUICellItem* ci					= (res->ItemsCount())?res->GetItemIdx(0):NULL;
	if(!ci)	
		return;

	SBuyItemInfo* pitem				= FindItem(ci);
	if(IsAddonAttached(pitem, at_silencer))
	{//detach
		SellItemAddons				(pitem,at_silencer);
	}else
	if(CanAttachAddon(pitem, at_silencer))
	{//attach
		shared_str addon_name		= GetAddonNameSect(pitem,at_silencer);

		if ( NULL==m_store_hierarchy->FindItem(addon_name) )
			return;
						
		SBuyItemInfo* addon_item	= CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
		bool b_res_addon			= TryToBuyItem(addon_item, bf_normal, pitem );
		if(!b_res_addon)
			DestroyItem				(addon_item);
	}
}

void CUIMpTradeWnd::OnBtnRifleScopeClicked(CUIWindow* w, void* d)
{
	CheckDragItemToDestroy				();
	CUIDragDropListEx*	res			= m_list[e_rifle];
	CUICellItem* ci					= (res->ItemsCount())?res->GetItemIdx(0):NULL;
	if(!ci)	
		return;

	SBuyItemInfo* pitem				= FindItem(ci);
	if(IsAddonAttached(pitem, at_scope))
	{//detach
		SellItemAddons				(pitem,at_scope);
	}else
	if(CanAttachAddon(pitem, at_scope))
	{//attach
		shared_str addon_name		= GetAddonNameSect(pitem,at_scope);

		if ( NULL==m_store_hierarchy->FindItem(addon_name) )
			return;
						
		SBuyItemInfo* addon_item	= CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
		bool b_res_addon			= TryToBuyItem(addon_item, bf_normal, pitem );
		if(!b_res_addon)
			DestroyItem				(addon_item);
	}
}

void CUIMpTradeWnd::OnBtnRifleGLClicked(CUIWindow* w, void* d)
{
	CheckDragItemToDestroy				();
	CUIDragDropListEx*	res			= m_list[e_rifle];
	CUICellItem* ci					= (res->ItemsCount())?res->GetItemIdx(0):NULL;
	if(!ci)	
		return;

	SBuyItemInfo* pitem				= FindItem(ci);
	if(IsAddonAttached(pitem, at_glauncher))
	{//detach
		SellItemAddons				(pitem,at_glauncher);
	}else
	if(CanAttachAddon(pitem, at_glauncher))
	{//attach
		shared_str addon_name		= GetAddonNameSect(pitem,at_glauncher);

		if ( NULL==m_store_hierarchy->FindItem(addon_name) )
			return;
						
		SBuyItemInfo* addon_item	= CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
		bool b_res_addon			= TryToBuyItem(addon_item, bf_normal, pitem );
		if(!b_res_addon)
			DestroyItem				(addon_item);
	}

	DeleteHelperItems				(m_list[e_rifle_ammo]);
	UpdateCorrespondingItemsForList (m_list[e_rifle]);
}

void CUIMpTradeWnd::OnBtnRifleAmmo2Clicked(CUIWindow* w, void* d)
{
}

bool CUIMpTradeWnd::TryToAttachItemAsAddon(SBuyItemInfo* itm, SBuyItemInfo* itm_parent)
{
	bool	b_res						= false;
	
	item_addon_type _addon_type			= GetItemType(itm->m_name_sect);
	if(_addon_type==at_not_addon)		return b_res;

	if(itm_parent)
	{
		if(CanAttachAddon(itm_parent,_addon_type))
		{
			return AttachAddon			(itm_parent, _addon_type);
		}
	}else // auto-attach
	for(u32 i=0; i<2; ++i)
	{
		u32 list_idx					= (i==0) ? e_rifle : e_pistol;
		CUIDragDropListEx*	_list		= m_list[list_idx];
		
		VERIFY							(_list->ItemsCount() <= 1);

		CUICellItem* ci					= (_list->ItemsCount()) ? _list->GetItemIdx(0) : NULL;
		if(!ci)	
			return	false;

		SBuyItemInfo* attach_to			= FindItem(ci);

		if(CanAttachAddon(attach_to,_addon_type))
		{
			AttachAddon					(attach_to,_addon_type);
			b_res						= true;
			break;
		}
	}		

	return				b_res;
}

void CUIMpTradeWnd::SellItemAddons(SBuyItemInfo* sell_itm, item_addon_type addon_type)
{
}

bool CUIMpTradeWnd::IsAddonAttached(SBuyItemInfo* itm, item_addon_type at)
{
	return			false;
}

bool CUIMpTradeWnd::CanAttachAddon(SBuyItemInfo* itm, item_addon_type at)
{
	return false;
}

SBuyItemInfo* CUIMpTradeWnd::DetachAddon(SBuyItemInfo* itm, item_addon_type at)
{
	return							CreateItem("", SBuyItemInfo::e_own, false);
}

shared_str CUIMpTradeWnd::GetAddonNameSect(SBuyItemInfo* itm, item_addon_type at)
{
	return 0;
}

bool CUIMpTradeWnd::AttachAddon(SBuyItemInfo* itm, item_addon_type at)
{
	return					true;
}

CUIMpTradeWnd::item_addon_type CUIMpTradeWnd::GetItemType(const shared_str& name_sect)
{
	const shared_str& group = g_mp_restrictions.GetItemGroup(name_sect);
	if(group=="scp")
		return		at_scope;
	else
	if(group=="sil")
		return		at_silencer;
	else
	if(group=="gl")
		return		at_glauncher;
	else
		return		at_not_addon;
}

u8 GetItemAddonsState_ext(SBuyItemInfo* item)
{
	return 0;
}

void SetItemAddonsState_ext(SBuyItemInfo* item, u8 addons)
{
}
