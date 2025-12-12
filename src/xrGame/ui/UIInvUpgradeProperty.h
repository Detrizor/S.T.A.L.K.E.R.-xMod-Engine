////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgradeProperty.h
//	Created 	: 22.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade property UIWindow class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "UIStatic.h"
#include "inventory_item.h"
#include "inventory_upgrade_property.h"

namespace inventory
{
	namespace upgrade
	{
		class Upgrade;
		class Property;
	}
}

class UIProperty : public CUIWindow
{
private:
	typedef CUIWindow                            inherited;
	typedef inventory::upgrade::Property         Property_type;
	typedef inventory::upgrade::Upgrade          Upgrade_type;
	typedef CInventoryItem::Upgrades_type        ItemUpgrades_type;
	typedef Property_type::FunctorParams_type    PropertyFunctorParams_type;

protected:
	shared_str		m_property_id;

	CUIStatic*		m_ui_icon;
	CUITextWnd*		m_ui_text;
	string256		m_text;

public:
					UIProperty();
	virtual			~UIProperty();
		void		init_from_xml( CUIXml& ui_xml );
		bool		init_property( shared_str const& property_id );
	Property_type*	get_property();

		bool		read_value_from_section( LPCSTR section, LPCSTR param, float& result );
		bool		compute_value( ItemUpgrades_type const& item_upgrades );
		bool		show_result( LPCSTR values );

}; // class UIProperty

// =========================================================================================

class UIInvUpgPropertiesWnd : public CUIWindow
{
private:
	typedef CUIWindow                    inherited;
	typedef inventory::upgrade::Upgrade  Upgrade_type;
	typedef xr_vector<UIProperty*>       Properties_type;
	typedef CInventoryItem::Upgrades_type  ItemUpgrades_type;

protected:
	Properties_type		m_properties_ui;
	ItemUpgrades_type	m_temp_upgrade_vector;
	CUIStatic*			m_Upgr_line;

public:
	UIInvUpgPropertiesWnd();
	~UIInvUpgPropertiesWnd() override;

	void initFromXml(CUIXml& xmlDoc);
	void initFromXml(LPCSTR strXmlName);

protected:
	void set_info(ItemUpgrades_type const& item_upgrades);

public:
	void set_upgrade_info(Upgrade_type& upgrade);
	void setInfo(PIItem item);
};
