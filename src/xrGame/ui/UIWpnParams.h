#pragma once
#include "UIWindow.h"

#include "UIDoubleProgressBar.h"

class CUIXml;
class CInventoryItem;
class CUICellItem;

#include "../../xrServerEntities/script_export_space.h"

class CUIWpnParams : public CUIWindow 
{
public:
							CUIWpnParams		();

	void 					InitFromXml			(CUIXml& xml_doc);
	void					SetInfo				(CUICellItem* item);

protected:
	CUITextWnd				m_text_barrel_length;
	CUITextWnd				m_text_barrel_length_value;
	CUITextWnd				m_textRPM;
	CUITextWnd				m_textRPMValue;
	CUITextWnd				m_textAmmoTypes;
	CUITextWnd				m_textAmmoTypesValue;
	CUIStatic				m_Prop_line;
};

// -------------------------------------------------------------------------------------------------

class CUIConditionParams : public CUIWindow 
{
public:
							CUIConditionParams	();

	void 					InitFromXml			(CUIXml& xml_doc);
	void					SetInfo				(CInventoryItem const* slot_wpn, CInventoryItem const& cur_wpn);

protected:
	CUIDoubleProgressBar	m_progress; // red or green
	CUIStatic				m_text;
};
