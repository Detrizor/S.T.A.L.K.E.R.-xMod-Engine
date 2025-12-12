#pragma once

#include "UIWindow.h"
#include "UIDoubleProgressBar.h"
#include "..\..\xrServerEntities\alife_space.h"

class CUIStatic;
class CUIDoubleProgressBar;
class CUIXml;
class CUICellItem;

enum EProtectionTypes {
	eBurnProtection = 0,
	eShockProtection,
	eChemBurnProtection,
	eRadiationProtection,
	eTelepaticProtection,
	eProtectionTypeMax
};

class CUIOutfitProtection : public CUIWindow
{
public:
					CUIOutfitProtection		();
	virtual			~CUIOutfitProtection	();

			void	InitFromXml		(CUIXml& xml_doc, LPCSTR base_str, u32 hit_type);
			void	SetValue		(float cur);

protected:
	CUIStatic		m_name; // texture + name
	CUITextWnd		m_value; // 100%
	float			m_magnitude;
}; // class CUIOutfitImmunity

// -------------------------------------------------------------------------------------

class CUIOutfitInfo : public CUIWindow
{
public:
					CUIOutfitInfo		();
	virtual			~CUIOutfitInfo		();

			void 	InitFromXml			(CUIXml& xml_doc);
			void 	setInfoSuit		(CUICellItem* itm);	
			void 	setInfoHelmet	(CUICellItem* itm);	
protected:
	CUIStatic*				m_Prop_line;
	CUIOutfitProtection*	m_items[eProtectionTypeMax];
	CUITextWnd*				m_textArmorClass;
	CUITextWnd*				m_textArmorClassValue;

}; // class CUIOutfitInfo