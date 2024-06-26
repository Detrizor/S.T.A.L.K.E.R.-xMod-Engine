#include "stdafx.h"
#include ".\uiradiobutton.h"
#include "UILines.h"

void CUIRadioButton::InitButton(Fvector2 pos, Fvector2 size)
{
	inherited::InitButton(pos, size);

	TextItemControl				();
    CUI3tButton::InitTexture	("ui_radio");
	Fvector2 sz					= m_background->Get(S_Enabled)->GetStaticItem()->GetSize(); 
	TextItemControl()->m_TextOffset.x = sz.x;

	CUI3tButton::InitButton		(pos, Fvector2().set(size.x, sz.y-5.0f));
}

void CUIRadioButton::InitTexture(LPCSTR tex_name)
{
	// do nothing
}