#pragma once
#include "UIStatic.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class CUICellItem;

class CUIAddonInfo : public CUIWindow
{
public:
	void initFromXml(CUIXml& xmlDoc);
	void setInfo(CUICellItem* itm);

private:
	CUIStatic	m_compatibleSlotsCap{};
	CUITextWnd	m_compatibleSlotsValue{};
	CUIStatic	m_availableSlotsCap{};
	CUITextWnd	m_availableSlotsValue{};
};
