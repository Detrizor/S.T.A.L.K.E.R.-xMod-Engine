#include "stdafx.h"
#include "uicursor.h"

#include "ui/UIStatic.h"
#include "ui/UIBtnHint.h"
#include "xrEngine/IInputReceiver.h"
#include "xrEngine/xr_input.h"

#define C_DEFAULT	D3DCOLOR_XRGB(0xff,0xff,0xff)

CUICursor::CUICursor()
:m_static(NULL),m_b_use_win_cursor(false)
{    
	bVisible				= false;
	vPrevPos.set			(0.0f, 0.0f);
	vPos.set				(0.f,0.f);
	InitInternal			();
	Device.seqRender.Add	(this,-3/*2*/);
	Device.seqResolutionChanged.Add(this);
}
//--------------------------------------------------------------------
CUICursor::~CUICursor	()
{
	xr_delete				(m_static);
	Device.seqRender.Remove	(this);
	Device.seqResolutionChanged.Remove(this);
}

void CUICursor::OnScreenResolutionChanged()
{
	xr_delete					(m_static);
	InitInternal				();
}

void CUICursor::Show()
{
	bVisible = true;
	::ClipCursor(nullptr);
}

void CUICursor::Hide()
{
	bVisible = false;
	pInput->ClipCursor(true);
}

void CUICursor::InitInternal()
{
	m_static					= xr_new<CUIStatic>();
	m_static->InitTextureEx		("ui\\ui_ani_cursor", "hud\\cursor");
	Frect						rect;
	rect.set					(0.f,0.f,128.f,128.f);
	m_static->SetTextureRect	(rect);

	m_static->SetWndSize		(Fvector2().set(24, 24));
	m_static->SetStretchTexture	(true);

	m_sys_metrics.x = GetSystemMetrics(SM_CXSCREEN);
	m_sys_metrics.y = GetSystemMetrics(SM_CYSCREEN);
	m_b_use_win_cursor	= true;		//--xd shoud do proper user setting
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;
void CUICursor::OnRender	()
{
	g_btnHint->OnRender();
	g_statHint->OnRender();

	if( !IsVisible() ) return;
#ifdef DEBUG
	VERIFY(last_render_frame != Device.dwFrame);
	last_render_frame = Device.dwFrame;

	if(bDebug)
	{
	CGameFont* F		= UI().Font().pFontDI;
	F->SetAligment		(CGameFont::alCenter);
	F->SetHeightI		(0.02f);
	F->OutSetI			(0.f,-0.9f);
	F->SetColor			(0xffffffff);
	Fvector2			pt = GetCursorPosition();
	F->OutNext			("%f-%f",pt.x, pt.y);
	}
#endif

	m_static->SetWndPos	(vPos);
	m_static->Update	();
	m_static->Draw		();
}

Fvector2 CUICursor::GetCursorPosition()
{
	return  vPos;
}

Fvector2 CUICursor::GetCursorPositionDelta()
{
	Fvector2 res_delta;

	res_delta.x = vPos.x - vPrevPos.x;
	res_delta.y = vPos.y - vPrevPos.y;
	return res_delta;
}

void CUICursor::UpdateCursorPosition(int _dx, int _dy)
{
	vPrevPos							= vPos;
	if (m_b_use_win_cursor)
	{
		Ivector2						pti;
		IInputReceiver::IR_GetMousePosReal(pti);
		vPos.x							= pti.x * (UI_BASE_WIDTH / m_sys_metrics.x);
		vPos.y							= pti.y * (UI_BASE_HEIGHT / m_sys_metrics.y);
	}
	else
	{
		float sens = 1.0f;		//--xd should implement real custom mouse sens for menus when not using system cursor
		vPos.x		+= _dx*sens;
		vPos.y		+= _dy*sens;
	}
	clamp		(vPos.x, 0.f, UI_BASE_WIDTH);
	clamp		(vPos.y, 0.f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(Fvector2 pos)
{
	vPos		= pos;
	POINT		p;
	p.x			= iFloor(vPos.x / (UI_BASE_WIDTH/(float)Device.dwWidth));
	p.y			= iFloor(vPos.y / (UI_BASE_HEIGHT/(float)Device.dwHeight));
	if (m_b_use_win_cursor)
		ClientToScreen(Device.m_hWnd, (LPPOINT)&p);
	SetCursorPos(p.x, p.y);
}

void CUICursor::resetCursorPosition()
{
	Fvector2 pos = { m_sys_metrics.x / 2.f, m_sys_metrics.y / 2.f };
	pos.div(UI().getScaleBasic());
	SetUICursorPosition(pos);
}
