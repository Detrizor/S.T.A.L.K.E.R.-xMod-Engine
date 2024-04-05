#include "stdafx.h"
#include "ui_base.h"
#include "GamePersistent.h"
#include "UICursor.h"
#include "ui/UIWindow.h"

extern	ENGINE_API	float			psUI_SCALE;

CUICursor&	GetUICursor				()	{return UI().GetUICursor();};
ui_core&	UI						()	{return *GamePersistent().m_pUI_core;};

void S2DVert::rotate_pt(const Fvector2& pivot, const float cosA, const float sinA, const float kx)
{
	Fvector2 t						= pt;
	t.sub							(pivot);
	pt.x							= t.x*cosA+t.y*sinA;
	pt.y							= t.y*cosA-t.x*sinA;
	pt.x							*= kx;
	pt.add							(pivot);
}
void C2DFrustum::CreateFromRect(const Frect& rect)
{
	m_rect.set						(float(rect.x1), float(rect.y1), float(rect.x2), float(rect.y2) );
	planes.resize					(4);
	planes[0].build					(rect.lt, Fvector2().set(-1, 0));
	planes[1].build					(rect.lt, Fvector2().set( 0,-1));
	planes[2].build					(rect.rb, Fvector2().set(+1, 0));
	planes[3].build					(rect.rb, Fvector2().set( 0,+1));
}

sPoly2D* C2DFrustum::ClipPoly	(sPoly2D& S, sPoly2D& D) const
{
	bool bFullTest		= false;
	for (u32 j=0; j<S.size(); j++)
	{
		if( !m_rect.in(S[j].pt) ) {
			bFullTest	= true;
			break		;
		}
	}

	sPoly2D*	src		= &D;
	sPoly2D*	dest	= &S;
	if(!bFullTest)		return dest;

	for (u32 i=0; i<planes.size(); i++)
	{
		// cache plane and swap lists
		const Fplane2 &P	= planes[i]	;
		std::swap			(src,dest)	;
		dest->clear			()			;

		// classify all points relative to plane #i
		float cls[UI_FRUSTUM_SAFE]	;
		for (u32 j=0; j<src->size(); j++) cls[j]=P.classify((*src)[j].pt);

		// clip everything to this plane
		cls[src->size()] = cls[0]	;
		src->push_back((*src)[0])	;
		Fvector2 dir_pt,dir_uv;		float denum,t;
		for (j=0; j<src->size()-1; j++)	{
			if ((*src)[j].pt.similar((*src)[j+1].pt,EPS_S)) continue;
			if (negative(cls[j]))	{
				dest->push_back((*src)[j])	;
				if (positive(cls[j+1]))	{
					// segment intersects plane
					dir_pt.sub((*src)[j+1].pt,(*src)[j].pt);
					dir_uv.sub((*src)[j+1].uv,(*src)[j].uv);
					denum = P.n.dotproduct(dir_pt);
					if (denum!=0) {
						t = -cls[j]/denum	; //VERIFY(t<=1.f && t>=0);
						dest->last().pt.mad	((*src)[j].pt,dir_pt,t);
						dest->last().uv.mad	((*src)[j].uv,dir_uv,t);
						dest->inc();
					}
				}
			} else {
				// J - outside
				if (negative(cls[j+1]))	{
					// J+1  - inside
					// segment intersects plane
					dir_pt.sub((*src)[j+1].pt,(*src)[j].pt);
					dir_uv.sub((*src)[j+1].uv,(*src)[j].uv);
					denum = P.n.dotproduct(dir_pt);
					if (denum!=0)	{
						t = -cls[j]/denum	; //VERIFY(t<=1.f && t>=0);
						dest->last().pt.mad	((*src)[j].pt,dir_pt,t);
						dest->last().uv.mad	((*src)[j].uv,dir_uv,t);
						dest->inc();
					}
				}
			}
		}

		// here we end up with complete polygon in 'dest' which is inside plane #i
		if (dest->size()<3) return 0;
	}
	return dest;
}

void ui_core::OnDeviceReset()
{
	m_device_res.set				(float(Device.dwWidth), float(Device.dwHeight));
	SetCurScale						(m_device_res);
	m_2DFrustum.CreateFromRect		(Frect().set(
									0.0f,
									0.0f,
									float(Device.dwWidth),
									float(Device.dwHeight)
									));
}

float ui_core::ClientToScreenScaledX(float left)	const
{
	return							left * GetScaleFactor();
}

float ui_core::ClientToScreenScaledY(float top)		const
{
	return							top * GetScaleFactor();
}

void ui_core::ClientToScreenScaled(Fvector2& dest, float left, float top)	const
{
	if (m_currentPointType != IUIRender::pttLIT)
		dest.set					(ClientToScreenScaledX(left),	ClientToScreenScaledY(top));
	else
		dest.set					(left,top);
}

void ui_core::ClientToScreenScaled(Fvector2& src_and_dest)	const
{
	if (m_currentPointType != IUIRender::pttLIT)
		src_and_dest.set			(ClientToScreenScaledX(src_and_dest.x),	ClientToScreenScaledY(src_and_dest.y));
}

void ui_core::AlignPixel(float& src_and_dest)	const
{
	if (m_currentPointType != IUIRender::pttLIT)
		src_and_dest				= (float)iFloor(src_and_dest);
}

void ui_core::PushScissor(const Frect& r_tgt, bool overlapped)
{
	if (UI().m_currentPointType == IUIRender::pttLIT)
		return;

	Frect r_top						= {0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT};
	Frect result					= r_tgt;
	if (!m_Scissors.empty() && !overlapped)
	{
		r_top						= m_Scissors.top();
	}
	if (!result.intersection(r_top,r_tgt))
			result.set				(0.0f,0.0f,0.0f,0.0f);

	if (!(result.x1 >= 0 && result.y1 >= 0 && result.x2 <= UI_BASE_WIDTH && result.y2 <= UI_BASE_HEIGHT))
	{
		Msg							("! r_tgt [%.3f][%.3f][%.3f][%.3f]", r_tgt.x1, r_tgt.y1, r_tgt.x2, r_tgt.y2);
		Msg							("! result [%.3f][%.3f][%.3f][%.3f]", result.x1, result.y1, result.x2, result.y2);
		VERIFY						(result.x1 >= 0 && result.y1 >= 0 && result.x2 <= UI_BASE_WIDTH && result.y2 <= UI_BASE_HEIGHT);
	}
	m_Scissors.push					(result);

	result.lt.x 					= ClientToScreenScaledX(result.lt.x);
	result.lt.y 					= ClientToScreenScaledY(result.lt.y);
	result.rb.x 					= ClientToScreenScaledX(result.rb.x);
	result.rb.y 					= ClientToScreenScaledY(result.rb.y);

	Irect							r;
	r.x1 							= iFloor(result.x1);
	r.x2 							= iFloor(result.x2+0.5f);
	r.y1 							= iFloor(result.y1);
	r.y2 							= iFloor(result.y2+0.5f);
	UIRender->SetScissor			(&r);
}

void ui_core::PopScissor()
{
	if (UI().m_currentPointType == IUIRender::pttLIT)
		return;

	VERIFY							(!m_Scissors.empty());
	m_Scissors.pop					();
	
	if (m_Scissors.empty())
		UIRender->SetScissor		(NULL);
	else
	{
		const Frect& top			= m_Scissors.top();
		Irect						tgt;
		tgt.lt.x 					= iFloor(ClientToScreenScaledX(top.lt.x));
		tgt.lt.y 					= iFloor(ClientToScreenScaledY(top.lt.y));
		tgt.rb.x 					= iFloor(ClientToScreenScaledX(top.rb.x));
		tgt.rb.y 					= iFloor(ClientToScreenScaledY(top.rb.y));

		UIRender->SetScissor		(&tgt);
	}
}

ui_core::ui_core()
{
	m_layout_unit					= pSettings->r_float("miscellaneous", "layout_unit");
	m_layout_factor					= pSettings->r_float("miscellaneous", "basic_layout_height") / m_layout_unit;
	m_text_scale_factor				= 1.f / m_layout_factor;
	OnDeviceReset					();

	if (!g_dedicated_server)
	{
		m_pUICursor					= xr_new<CUICursor>();
		m_pFontManager				= xr_new<CFontManager>();
	}
	else
	{
		m_pUICursor					= NULL;
		m_pFontManager				= NULL;
	}
	m_bPostprocess					= false;
	m_currentPointType				= IUIRender::pttTL;
}

ui_core::~ui_core()
{
	xr_delete						(m_pFontManager);
	xr_delete						(m_pUICursor);
}

void ui_core::pp_start()
{
	m_bPostprocess					= true;

	m_pp_res.set					(float(::Render->getTarget()->get_width()), float(::Render->getTarget()->get_height()));
	m_2DFrustumPP.CreateFromRect	(Frect().set(
									0.0f,
									0.0f,
									float(::Render->getTarget()->get_width()),
									float(::Render->getTarget()->get_height())
									));

	SetCurScale						(m_pp_res);
}

void ui_core::pp_stop()
{
	m_bPostprocess					= false;
	SetCurScale						(m_device_res);
}

void ui_core::RenderFont()
{
	Font().Render					();
}

bool ui_core::is_widescreen()
{
	return							false;
}

float ui_core::get_current_kx()
{
	return							1.f;
}

shared_str	ui_core::get_xml_name(LPCSTR fn)
{
	string_path						str;
	xr_sprintf						(str, "%s", fn);
	if (!strext(fn))
		xr_strcat					(str, ".xml");
	return							str;
}

float ui_core::GetScale(EScaling scaling) const
{
	switch (scaling)
	{
	case sAbsolute:
		return						1.f;
	case sScreenHeight:
		return						m_height_scale / GetScaleFactor();
	case sScreenWidth:
		return						m_width_scale / GetScaleFactor();
	case sScreenHeightLayout:
		return						m_height_scale_layout / GetScaleFactor();
	case sScreenWidthLayout:
		return						m_width_scale_layout / GetScaleFactor();
	default:
		FATAL						(shared_str().printf("incorrect scaling [%d]", scaling).c_str());
		return						false;
	}
}

float ui_core::GetScaleFactor() const
{
	return							psUI_SCALE * m_layout_factor;
}

void ui_core::SetCurScale(const Fvector2& res)
{
	m_height_scale					= res.y;
	m_width_scale					= res.x;
	m_height_scale_layout			= res.y / m_layout_unit;
	m_width_scale_layout			= res.x / m_layout_unit;
}
