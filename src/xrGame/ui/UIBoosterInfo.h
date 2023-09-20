#pragma once
#include "UIWindow.h"
#include "../EntityCondition.h"

class CUIXml;
class CUIStatic;
class CUITextWnd;
class UIBoosterInfoItem;
class CUICellItem;

class CUIBoosterInfo : public CUIWindow
{
public:
					CUIBoosterInfo		();
	virtual			~CUIBoosterInfo		();
			void	InitFromXml				(CUIXml& xml);
			void	SetInfo					(CUICellItem* itm);

protected:
	UIBoosterInfoItem*	m_boosts[eBoostMaxCount];
	UIBoosterInfoItem*	m_need_hydration;
	UIBoosterInfoItem*	m_need_satiety;
	UIBoosterInfoItem*	m_health_outer;
	UIBoosterInfoItem*	m_health_neural;
	UIBoosterInfoItem*	m_power_short;
	UIBoosterInfoItem*	m_booster_anabiotic;

	UIBoosterInfoItem*	m_bullet_speed;
	UIBoosterInfoItem*	m_armor_piercing;
	UIBoosterInfoItem*	m_bullet_pulse;

	UIBoosterInfoItem*	m_ammo_type;
	UIBoosterInfoItem*	m_capacity;

	CUIStatic*			m_Prop_line;

}; // class CUIBoosterInfo

// -----------------------------------

class UIBoosterInfoItem : public CUIWindow
{
public:
				UIBoosterInfoItem	();
	virtual		~UIBoosterInfoItem();
		
		void	Init				( CUIXml& xml, LPCSTR section );
		void	SetCaption			( LPCSTR name );
		void	SetValue			( float value );
		void	SetStrValue			( LPCSTR value );
	
private:
	CUIStatic*	m_caption;
	CUITextWnd*	m_value;
	float		m_magnitude;
	bool		m_show_sign;
	bool		m_perc_unit;
	shared_str	m_unit_str;
	shared_str	m_texture_minus;
	shared_str	m_texture_plus;

}; // class UIBoosterInfoItem
