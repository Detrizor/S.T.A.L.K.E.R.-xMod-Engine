#pragma once

#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CUICellItem;
class CUIMiscInfoItem;

class CUIAmmoInfo : public CUIWindow
{
public:
	CUIAmmoInfo();
	~CUIAmmoInfo() override;

	void initFromXml(CUIXml& xmlDoc);

public:
	void setInfo(CUICellItem* pCellItem);

private:
	xptr<CUIStatic>			m_pDisclaimer{};
	xptr<CUIMiscInfoItem>	m_pBulletSpeed{ this };
	xptr<CUIMiscInfoItem>	m_pBulletPulse{ this };
	xptr<CUIMiscInfoItem>	m_pArmorPiercing{ this };
	xptr<CUIMiscInfoItem>	m_pImpair{ this };
};
