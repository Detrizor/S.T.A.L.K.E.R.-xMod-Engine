#pragma once
#include "ui_defs.h"

class CUICursor;
class CUIGameCustom;
enum EScaling;

class CDeviceResetNotifier :public pureDeviceReset
{
public:
						CDeviceResetNotifier					()	{Device.seqDeviceReset.Add(this,REG_PRIORITY_NORMAL);};
	virtual				~CDeviceResetNotifier					()	{Device.seqDeviceReset.Remove(this);};
	virtual void		OnDeviceReset							()	{};

};

struct CFontManager :public pureDeviceReset			{
							CFontManager			();
							~CFontManager			();

	typedef xr_vector<CGameFont**>					FONTS_VEC;
	typedef FONTS_VEC::iterator						FONTS_VEC_IT;
	FONTS_VEC				m_all_fonts;
	void					Render					();

	// hud font
	CGameFont*				pFontMedium;
	CGameFont*				pFontDI;

	CGameFont*				pFontArial14;
	CGameFont*				pFontGraffiti19Russian;
	CGameFont*				pFontGraffiti22Russian;
	CGameFont*				pFontLetterica16Russian;
	CGameFont*				pFontLetterica18Russian;
	CGameFont*				pFontGraffiti32Russian;
	CGameFont*				pFontGraffiti50Russian;
	CGameFont*				pFontLetterica25;
	CGameFont*				pFontStat;

	void					InitializeFonts			();
	void					InitializeFont			(CGameFont*& F, LPCSTR section, u32 flags = 0);

	virtual void			OnDeviceReset			();
};

class ui_core: public CDeviceResetNotifier
{
	C2DFrustum		m_2DFrustum;
	C2DFrustum		m_2DFrustumPP;
	C2DFrustum		m_FrustumLIT;

	bool			m_bPostprocess;

	CFontManager*	m_pFontManager;
	CUICursor*		m_pUICursor;

public:
	xr_stack<Frect> m_Scissors;
	
					ui_core							();
					~ui_core						();
	CFontManager&	Font							()								{return *m_pFontManager;}
	CUICursor&		GetUICursor						()								{return *m_pUICursor;}

	float			ClientToScreenScaledX			(float left)								const;
	float			ClientToScreenScaledY			(float top)									const;
	void			ClientToScreenScaled			(Fvector2& dest, float left, float top)		const;
	void			ClientToScreenScaled			(Fvector2& src_and_dest)					const;
	void			AlignPixel						(float& src_and_dest)						const;

	const C2DFrustum& ScreenFrustum					()	const						{return (m_bPostprocess)?m_2DFrustumPP:m_2DFrustum;}
	C2DFrustum&		ScreenFrustumLIT				()								{return m_FrustumLIT;}
	void			PushScissor						(const Frect& r, bool overlapped=false);
	void			PopScissor						();

	void			pp_start						();
	void			pp_stop							();
	void			RenderFont						();

	virtual void	OnDeviceReset					();
	static	bool	is_widescreen					();
	static	float	get_current_kx					();
	shared_str		get_xml_name					(LPCSTR fn);
	
	IUIRender::ePointType		m_currentPointType;

private:
			Fvector2		m_device_res;
			Fvector2		m_pp_res;
			float			m_height_scale;
			float			m_height_scale_layout;
			float			m_width_scale;
			float			m_width_scale_layout;
			float			m_layout_unit;
			float			m_layout_factor;
			float			m_text_scale_factor;

			void			SetCurScale				(const Fvector2& res);

public:
	float								GetTextScaleFactor					C$	()		{ return m_text_scale_factor; }

	float								GetScale							C$	(EScaling scaling);
	float								GetScaleFactor						C$	();
};

extern CUICursor&		GetUICursor				();
extern ui_core&			UI						();
extern CUIGameCustom*	CurrentGameUI			();
