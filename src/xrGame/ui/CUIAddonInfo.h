#pragma once
#include "UIStatic.h"

class CUIXml;
class CUICellItem;
class CUIMiscInfoItem;

class CUIAddonInfo : public CUIWindow
{
public:
	CUIAddonInfo();
	~CUIAddonInfo() override;

	void initFromXml(CUIXml& xmlDoc);
	void setInfo(CUICellItem* itm);

private:
	xptr<CUIMiscInfoItem>	m_pCompatibleSlots{ this };
	xptr<CUIMiscInfoItem>	m_pAvailableSlots{ this };
};
