#pragma once

#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;

class CUIMiscInfoItem : public CUIWindow
{
public:
	CUIMiscInfoItem(CUIWindow* pParent);
	~CUIMiscInfoItem() override;

	void init(CUIXml& xmlDoc, LPCSTR strSection, bool bAttach);

public:
	void setCaption(LPCSTR strCaption);
	void setValue(float fValue, float& h);
	void setStrValue(LPCSTR strValue, float& h);

private:
	CUIWindow* const m_pParent;

	bool m_bAttaching{ true };

	float	m_fMagnitude{ 1.F };
	bool	m_bShowSign{ false };
	bool	m_bPercUnit{ false };

	xptr<CUIStatic>		m_pIcon{};
	xptr<CUITextWnd>	m_pCaption{};
	xptr<CUITextWnd>	m_pValue{};

	shared_str m_strUnit{};
};
