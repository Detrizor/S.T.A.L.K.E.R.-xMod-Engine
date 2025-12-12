#pragma once

#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;

class CUIMiscInfoItem : public CUIWindow
{
public:
	void init(CUIXml& xmlDoc, LPCSTR strSection);
	void SetCaption(LPCSTR strCaption);
	void SetValue(float fValue);
	void SetStrValue(LPCSTR strValue);

private:
	float	m_fMagnitude{ 1.F };
	bool	m_bShowSign{ false };
	bool	m_bPercUnit{ false };

	CUIStatic* m_pCaption{ nullptr };
	CUITextWnd* m_pValue{ nullptr };

	shared_str m_strUnit{};
	shared_str m_strTextureMinus{};
	shared_str m_strTexturePlus{};
};
