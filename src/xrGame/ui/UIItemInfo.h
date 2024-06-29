#pragma once
#include "uiwindow.h"


class CInventoryItem;
class CUIStatic;
class CUITextWnd;
class CUIScrollView;
class CUIProgressBar;
class CUIConditionParams;
class CUIWpnParams;
class CUIArtefactParams;
class CUIFrameWindow;
class UIInvUpgPropertiesWnd;
class CUIOutfitInfo;
class CUIBoosterInfo;
class CUIAddonInfo;
class CUICellItem;

extern const char * const 		fieldsCaptionColor;

class CUIItemInfo: public CUIWindow
{
private:
	typedef CUIWindow inherited;
	struct _desc_info
	{
		CGameFont*			pDescFont;
		u32					uDescClr;
		bool				bShowDescrText;
	};
	_desc_info				m_desc_info;
	CInventoryItem* m_pInvItem;
public:
						CUIItemInfo			();
	virtual				~CUIItemInfo		();
	CInventoryItem*		CurrentItem			() const {return m_pInvItem;}
	void				InitItemInfo		(Fvector2 pos, Fvector2 size, LPCSTR xml_name);
	void				InitItemInfo		(LPCSTR xml_name);
	void				InitItem			(CUICellItem* pCellItem, u32 item_price = u32(-1), LPCSTR trade_tip = NULL);

	void				TryAddWpnInfo		(CUICellItem* item);
	void				TryAddArtefactInfo	(CUICellItem* item);
	void				TryAddOutfitInfo	(CUICellItem* item);
	void				TryAddUpgradeInfo	(CUICellItem* item);
	void				TryAddBoosterInfo	(CUICellItem* item);
	void				tryAddAddonInfo		(CUICellItem* itm);
	
	virtual void		Draw				();
	bool				m_b_FitToHeight;
	u32					delay;
	
	CUIFrameWindow*		UIBackground;
	CUITextWnd*			UIName;
	CUITextWnd*			UIWeight;
	CUITextWnd*			UIVolume;
	CUITextWnd*			UICondition;
	CUITextWnd*			UIAmount;
	CUITextWnd*			UICost;
	CUITextWnd*			UITradeTip;
	CUIScrollView*		UIDesc;

	CUIWpnParams*			UIWpnParams;
	CUIArtefactParams*		UIArtefactParams;
	UIInvUpgPropertiesWnd*	UIProperties;
	CUIOutfitInfo*			UIOutfitInfo;
	CUIBoosterInfo*			UIBoosterInfo;

	Fvector2			UIItemImageSize; 
	CUIStatic*			UIItemImage;

private:
	xptr<CUIAddonInfo>					m_addon_info							= nullptr;
};
