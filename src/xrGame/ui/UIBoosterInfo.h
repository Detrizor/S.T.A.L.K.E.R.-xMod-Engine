#pragma once

#include "UIWindow.h"
#include "../EntityCondition.h"

class CUIXml;
class CUICellItem;
class CUIMiscInfoItem;

class CUIBoosterInfo : public CUIWindow
{
public:
	CUIBoosterInfo();
	~CUIBoosterInfo() override;

	void initFromXml(CUIXml& xmlDoc);

public:
	void setInfo(CUICellItem* pCellItem);

protected:
	xarr<xptr<CUIMiscInfoItem>, eBoostMaxCount> m_pBoosts{ this };

	xptr<CUIMiscInfoItem> m_pNeedHydration{ this };
	xptr<CUIMiscInfoItem> m_pNeedSatiety{ this };
	xptr<CUIMiscInfoItem> m_pHealthOuter{ this };
	xptr<CUIMiscInfoItem> m_pHealthNeural{ this };
	xptr<CUIMiscInfoItem> m_pPowerShort{ this };
	xptr<CUIMiscInfoItem> m_pBoosterAnabiotic{ this };
};
