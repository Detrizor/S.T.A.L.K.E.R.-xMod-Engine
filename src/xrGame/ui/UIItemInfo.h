#pragma once

#include "uiwindow.h"

class CUIStatic;
class CUITextWnd;
class CUIAmmoInfo;
class CUIMiscInfo;
class CUICellItem;
class CUIWpnParams;
class CUIAddonInfo;
class CUIOutfitInfo;
class CUIScrollView;
class CUIBoosterInfo;
class CUIFrameWindow;
class CUIArtefactParams;
class UIInvUpgPropertiesWnd;

class CInventoryItem;

extern LPCSTR const fieldsCaptionColor;

class CUIItemInfo: public CUIWindow
{
	using _super = CUIWindow;

	struct _desc_info
	{
		xptr<CUITextWnd>	text{ nullptr };
		bool bFitToHeight	{ false };
		CGameFont*			pDescFont{};
		u32					uDescClr{};
	};

public:
	CUIItemInfo();
	~CUIItemInfo() override;

	void init(Fvector2 pos, Fvector2 size, LPCSTR xml_name);
	void init(LPCSTR strXmlName);

private:
	void set_amount_info(CUICellItem* pCellItem, CInventoryItem* pItem);
	void set_description_info(CUICellItem* pCellItem, CInventoryItem* pItem, bool bTradeTip);
	void set_custom_info(CUICellItem* pCellItem, CInventoryItem* pItem);

public:
	CUICellItem* getCurItem() const { return m_pCurItem; }

	void setItem(CUICellItem* pCellItem, u32 item_price = u32_max, LPCSTR trade_tip = nullptr);

	void Draw() override;
	
private:
	CUICellItem* m_pCurItem{ nullptr };

	xptr<CUIFrameWindow>	m_pFrame{};
	xptr<CUIStatic>			m_pBackground{};

	xptr<CUITextWnd>	m_pUIName{};
	xptr<CUITextWnd>	m_pUIWeight{};
	xptr<CUITextWnd>	m_pUIVolume{};
	xptr<CUITextWnd>	m_pUICondition{};
	xptr<CUITextWnd>	m_pUIAmount{};
	xptr<CUITextWnd>	m_pUICost{};
	xptr<CUITextWnd>	m_pUITradeTip{};

	xptr<CUIScrollView>		m_pUIDesc{};
	_desc_info				m_descInfo{};

	xptr<CUIOutfitInfo>				m_pUIOutfitInfo{};
	xptr<CUIWpnParams>				m_pUIWpnParams{};
	xptr<CUIArtefactParams>			m_pUIArtefactParams{};
	xptr<CUIAddonInfo>				m_pUIAddonInfo{};
	xptr<CUIAmmoInfo>				m_pUIAmmoInfo{};
	xptr<CUIBoosterInfo>			m_pUIBoosterInfo{};
	xptr<CUIMiscInfo>				m_pUIMiscInfo{};
	xptr<UIInvUpgPropertiesWnd>		m_pUIInvUpgProperties{};

	xptr<CUIStatic>		m_pUIItemImage{ nullptr };
	Fvector2			m_UIItemImageSize{ 0.F, 0.F };
};
