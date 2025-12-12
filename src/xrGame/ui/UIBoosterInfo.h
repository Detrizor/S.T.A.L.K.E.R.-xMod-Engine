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
	xptr<CUIMiscInfoItem> m_boosts[eBoostMaxCount]{};
	xptr<CUIMiscInfoItem> m_need_hydration{};
	xptr<CUIMiscInfoItem> m_need_satiety{};
	xptr<CUIMiscInfoItem> m_health_outer{};
	xptr<CUIMiscInfoItem> m_health_neural{};
	xptr<CUIMiscInfoItem> m_power_short{};
	xptr<CUIMiscInfoItem> m_booster_anabiotic{};
};
