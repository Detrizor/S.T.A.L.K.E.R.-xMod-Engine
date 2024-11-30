#pragma once
#include "UIStatic.h"

class CUIXml;
class CUICellItem;
class CUITextWnd;
class CUIStatic;

class CUIAddonInfo : public CUIWindow
{
public:
	void 								initFromXml								(CUIXml& xml_doc);
	void								setInfo									(CUICellItem* itm);

private:
	CUIStatic							m_compatible_slots_cap;
	CUITextWnd							m_compatible_slots_value;
	CUIStatic							m_available_slots_cap;
	CUITextWnd							m_available_slots_value;
};
