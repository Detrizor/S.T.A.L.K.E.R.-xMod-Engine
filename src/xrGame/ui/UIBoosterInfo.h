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
			void	InitFromXml				(CUIXml& xml);
			void	SetInfo					(CUICellItem* itm);

protected:
	xptr<UIBoosterInfoItem>	m_boosts[eBoostMaxCount];
	xptr<UIBoosterInfoItem>	m_need_hydration;
	xptr<UIBoosterInfoItem>	m_need_satiety;
	xptr<UIBoosterInfoItem>	m_health_outer;
	xptr<UIBoosterInfoItem>	m_health_neural;
	xptr<UIBoosterInfoItem>	m_power_short;
	xptr<UIBoosterInfoItem>	m_booster_anabiotic;
	
	xptr<CUIStatic>			m_disclaimer;
	xptr<UIBoosterInfoItem>	m_bullet_speed;
	xptr<UIBoosterInfoItem>	m_bullet_pulse;
	xptr<UIBoosterInfoItem>	m_armor_piercing;
	xptr<UIBoosterInfoItem>	m_impair;

	xptr<UIBoosterInfoItem>	m_ammo_type;
	xptr<UIBoosterInfoItem>	m_magazine_capacity;

	xptr<UIBoosterInfoItem>	m_capacity;
	xptr<UIBoosterInfoItem>	m_artefact_isolation;
	xptr<UIBoosterInfoItem>	m_max_artefact_radiation_limit;
	xptr<UIBoosterInfoItem>	m_radiation;

	xptr<CUIStatic>			m_Prop_line;

}; // class CUIBoosterInfo

// -----------------------------------

class UIBoosterInfoItem : public CUIWindow
{
public:
	void								Init									(CUIXml& xml, LPCSTR section);
	void								SetCaption								(LPCSTR name);
	void								SetValue								(float value);
	void								SetStrValue								(LPCSTR value);
	
private:
	CUIStatic*							m_caption								= nullptr;
	CUITextWnd*							m_value									= nullptr;
	float								m_magnitude								= 1.f;
	bool								m_show_sign								= false;
	bool								m_perc_unit								= false;
	shared_str							m_unit_str								= "";
	shared_str							m_texture_minus							= "";
	shared_str							m_texture_plus							= "";

};
