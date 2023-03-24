#pragma once

#pragma warning (disable : 4511 )
#pragma warning (disable : 4512 )

#include "../ui_base.h"
#include <boost/noncopyable.hpp>

typedef CGameFont::EAligment ETextAlignment;

typedef enum {
	valTop = 0,
	valCenter,
	valBotton
} EVTextAlignment;

enum EScaling
{
	sAbsolute,
	sScreenHeight,
	sScreenWidth
};

class ITextureOwner
{
public:
	virtual				~ITextureOwner			()												{}	
	virtual void		InitTexture				(LPCSTR texture)								= 0;
	virtual void		InitTextureEx			(LPCSTR texture, LPCSTR shader)					= 0;
	virtual void		SetTextureRect			(const Frect& r)								= 0;
	virtual const Frect& GetTextureRect			()										const	= 0;
	virtual void		SetTextureColor			(u32 color)										= 0;
	virtual u32			GetTextureColor			()										const	= 0;
	virtual void		SetStretchTexture		(bool stretch)									= 0;
	virtual bool		GetStretchTexture		()												= 0;
	
	virtual void		SetTextureMargin		(float margin)									{};
};

class CUISimpleWindow : public boost::noncopyable
{
public:
							CUISimpleWindow			()										{m_wndPos.set(0,0); m_pos_scaling.set(sAbsolute, sAbsolute); m_wndSize.set(0,0); m_size_scaling.set(sAbsolute, sAbsolute); m_clamp=false; m_scale=1.f;}

	virtual void			SetWndPos				(const Fvector2& pos)					{SetX(pos.x); SetY(pos.y);}
	virtual void			SetX					(const float& x)						{m_wndPos.x = x; m_wndPos.x /= GetScale(m_pos_scaling.x);}
	virtual void			SetY					(const float& y)						{m_wndPos.y = y; m_wndPos.y /= GetScale(m_pos_scaling.y);}

	virtual void			SetWndSize				(const Fvector2& size)					{SetWidth(size.x); SetHeight(size.y);}
	virtual void			SetWidth				(const float& width)					{m_wndSize.x = width; m_wndSize.x /= GetScale(m_size_scaling.x);}
	virtual void			SetHeight				(const float& height)					{m_wndSize.y = height; m_wndSize.y /= GetScale(m_size_scaling.y);}

	virtual void			SetWndRect				(const Frect& rect)						{SetWndPos(rect.lt); SetWidth(rect.width()); SetHeight(rect.height());}

			void			MoveWndDelta			(float dx, float dy)					{MoveWndDelta(Fvector2().set(dx, dy));}
			void			MoveWndDelta			(const Fvector2& d)						{Fvector2 pos = GetWndPos(); pos.add(d); SetWndPos(pos);}

			Fvector2		GetWndPos				()								const	{Fvector2 pos; pos.set(GetX(), GetY()); return pos;}
			float			GetX					()								const	{float x = m_wndPos.x; x *= m_scale * GetScale(m_pos_scaling.x); if (m_clamp) clamp(x, 0.f, UI_BASE_WIDTH); return x;}
			float			GetY					()								const	{float y = m_wndPos.y; y *= m_scale * GetScale(m_pos_scaling.y); if (m_clamp) clamp(y, 0.f, UI_BASE_HEIGHT); return y;}
			Fvector2		GetWndSize				()								const	{Fvector2 size; size.set(GetWidth(), GetHeight()); return size;}
			float			GetWidth				()								const	{float width = m_wndSize.x; width *= m_scale * GetScale(m_size_scaling.x); if (m_clamp) clamp(width, 0.f, UI_BASE_WIDTH); return width;}
			float			GetHeight				()								const	{float height = m_wndSize.y; height *= m_scale * GetScale(m_size_scaling.y); if (m_clamp) clamp(height, 0.f, UI_BASE_HEIGHT); return height;}

	IC		void			SetVisible				(bool vis)								{m_bShowMe = vis;}
	IC		bool			GetVisible				()								const	{return m_bShowMe;}

private:
	bool					m_bShowMe;
	Fvector2				m_wndPos;
	Fvector2				m_wndSize;

private:
	_vector2<EScaling>		m_pos_scaling;
	_vector2<EScaling>		m_size_scaling;
			bool			m_clamp;

public:
	IC const float			GetScale				(const EScaling& sc)			const	{ return UI().GetScale(sc); }
			void			CopyPosSize				(const CUISimpleWindow& from)			{ m_wndPos = from.m_wndPos; m_wndSize = from.m_wndSize; m_pos_scaling = from.m_pos_scaling; m_size_scaling = from.m_size_scaling; }
			void			SetClamp				(const bool& val)						{ m_clamp = val; }
			void			SetPosSize				(const u8& idx, const float& val, const EScaling& sc)
																							{
																								switch (idx)
																								{
																								case 0: m_wndPos.x = val; m_pos_scaling.x = sc; break;
																								case 1: m_wndPos.y = val; m_pos_scaling.y = sc; break;
																								case 2: m_wndSize.x = val; m_size_scaling.x = sc; break;
																								case 3: m_wndSize.y = val; m_size_scaling.y = sc; break;
																								}
																							}

protected:
			float			m_scale;
};

class CUISelectable
{
protected:
	bool m_bSelected;
public:
							CUISelectable		()							:m_bSelected(false){}
	bool					GetSelected			() const					{return m_bSelected;}
	virtual void			SetSelected			(bool b)					{m_bSelected = b;};
};