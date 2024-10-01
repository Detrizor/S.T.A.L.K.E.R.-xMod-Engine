////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_upgrade.cpp
//	Created 	: 27.11.2007
//  Modified 	: 27.11.2007
//	Author		: Sokolov Evgeniy
//	Description : Inventory item upgrades class impl
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "inventory_item.h"
#include "inventory_item_impl.h"

#include "ai_object_location.h"
#include "object_broker.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "inventory_upgrade_manager.h"
#include "inventory_upgrade.h"
#include "Level.h"
#include "WeaponMagazinedWGrenade.h"

bool CInventoryItem::has_upgrade_group( const shared_str& upgrade_group_id )
{
	Upgrades_type::iterator it	= m_upgrades.begin();
	Upgrades_type::iterator it_e	= m_upgrades.end();

	for(; it!=it_e; ++it)
	{
		inventory::upgrade::Upgrade* upgrade = ai().alife().inventory_upgrade_manager().get_upgrade( *it );
		if(upgrade->parent_group_id()==upgrade_group_id)
			return true;
	}
	return false;
}

bool CInventoryItem::has_upgrade( const shared_str& upgrade_id )
{
	if ( m_section_id == upgrade_id )
	{
		return true;
	}
	return ( std::find( m_upgrades.begin(), m_upgrades.end(), upgrade_id ) != m_upgrades.end() );
}

void CInventoryItem::add_upgrade( const shared_str& upgrade_id, bool loading )
{
	if ( !has_upgrade( upgrade_id ) )
	{
		m_upgrades.push_back( upgrade_id );

		if ( !loading )
		{
			NET_Packet					P;
			CGameObject::u_EventGen		( P, GE_INSTALL_UPGRADE, object_id() );
			P.w_stringZ					( upgrade_id );
			CGameObject::u_EventSend	( P );
		}
	}
}

bool CInventoryItem::get_upgrades_str( string2048& res ) const
{
	int prop_count = 0;
	res[0] = 0;
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	inventory::upgrade::Upgrade* upgr;
	for ( ; ib != ie; ++ib )
	{
		upgr = ai().alife().inventory_upgrade_manager().get_upgrade( *ib );
		if ( !upgr ) { continue; }

		LPCSTR upgr_section = upgr->section();
		if ( prop_count > 0 )
		{
			xr_strcat( res, sizeof(res), ", " );
		}
		xr_strcat( res, sizeof(res), upgr_section );
		++prop_count;
	}
	if ( prop_count > 0 )
	{
		return true;
	}
	return false;
}

bool CInventoryItem::equal_upgrades( Upgrades_type const& other_upgrades ) const
{
	if ( m_upgrades.size() != other_upgrades.size() )
	{
		return false;
	}
	
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	for ( ; ib != ie; ++ib )
	{
		shared_str const& name1 = (*ib);
		bool upg_equal = false;
		Upgrades_type::const_iterator ib2 = other_upgrades.begin();
		Upgrades_type::const_iterator ie2 = other_upgrades.end();
		for ( ; ib2 != ie2; ++ib2 )
		{
			if ( name1.equal( (*ib2) ) )
			{
				upg_equal = true;
				break;//from for2, in for1
			}
		}
		if ( !upg_equal )
		{
			return false;
		}
	}
	return true;
}

#ifdef DEBUG	
void CInventoryItem::log_upgrades()
{
	Msg( "* all upgrades of item = %s", m_section_id.c_str() );
	Upgrades_type::const_iterator ib = m_upgrades.begin();
	Upgrades_type::const_iterator ie = m_upgrades.end();
	for ( ; ib != ie; ++ib )
	{
		Msg( "    %s", (*ib).c_str() );
	}
	Msg( "* finish - upgrades of item = %s", m_section_id.c_str() );
}
#endif // DEBUG

void CInventoryItem::net_Spawn_install_upgrades( Upgrades_type saved_upgrades ) // net_Spawn
{
	m_upgrades.clear_not_free();

	if ( !ai().get_alife() )
	{
		return;
	}
	
	ai().alife().inventory_upgrade_manager().init_install( *this ); // from pSettings

	Upgrades_type::iterator ib = saved_upgrades.begin();
	Upgrades_type::iterator ie = saved_upgrades.end();
	for ( ; ib != ie ; ++ib )
	{
		ai().alife().inventory_upgrade_manager().upgrade_install( *this, (*ib), true );
	}
}

bool CInventoryItem::install_upgrade( LPCSTR section )
{
	return install_upgrade_impl( section, false );
}

bool CInventoryItem::verify_upgrade( LPCSTR section )
{
	return install_upgrade_impl( section, true );
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, float& value, bool test)
{
	if (!pSettings->line_exist(section, name))
		return false;
	
	if (!test)
	{
		xr_string str				= pSettings->r_string(section, name);
		LPCSTR c_str				= str.c_str();
		u32 len						= xr_strlen(c_str);
		xr_string type_l			= (len > 1) ? str.substr(0, 2) : "";
		xr_string type				= (len > 0) ? str.substr(0, 1) : "";
		float dvalue_l				= (len > 2) ? (float)atof(str.substr(2).c_str()) : 0.f;
		float dvalue				= (len > 1) ? (float)atof(str.substr(1).c_str()) : 0.f;
		LPCSTR process_type_l		= type_l.c_str();
		LPCSTR process_type			= type.c_str();
		if (!xr_strcmp(process_type_l, "*+"))
			value					+= value * dvalue_l;
		else if (!xr_strcmp(process_type_l, "*-"))
			value					-= value * dvalue_l;
		else if (!xr_strcmp(process_type, "*"))
			value					*= dvalue;
		else if (!xr_strcmp(process_type, "+"))
			value					+= dvalue;
		else if (!xr_strcmp(process_type, "-"))
			value					-= dvalue;
		else
		{
			if (!xr_strcmp(c_str, "true") || !xr_strcmp(c_str, "on"))
				value				= 1.f;
			else if (!xr_strcmp(c_str, "false") || !xr_strcmp(c_str, "off"))
				value				= 0.f;
			else
				value				= (float)atof(c_str);
		}
	}

	return true;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, int& value, bool test)
{
	float tmp		= (float)value;
	bool result		= process_if_exists(section, name, tmp, test);
	if (result && !test)
		value		= (int)tmp;
	return			result;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, u32& value, bool test)
{
	float tmp		= (float)value;
	bool result		= process_if_exists(section, name, tmp, test);
	if (result && !test)
		value		= (u32)tmp;
	return			result;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, u8& value, bool test)
{
	float tmp		= (float)value;
	bool result		= process_if_exists(section, name, tmp, test);
	if (result && !test)
		value		= (u8)tmp;
	return			result;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, bool& value, bool test)
{
	float tmp;
	bool result		= process_if_exists(section, name, tmp, test);
	if (result && !test)
		value		= (bool)tmp;
	return			result;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, shared_str& value, bool test)
{
	if (!pSettings->line_exist(section, name))
		return false;
	
	if (!test)
	{
		xr_string str		= pSettings->r_string(section, name);
		value._set			(str.c_str());
	}

	return true;
}

bool CInventoryItem::process_if_exists(LPCSTR section, LPCSTR name, LPCSTR& value, bool test)
{
	shared_str		tmp;
	bool result		= process_if_exists(section, name, tmp, test);
	if (result && !test)
		value		= tmp.c_str();
	return			result;
}

bool CInventoryItem::install_upgrade_impl(LPCSTR section, bool test)
{
	u32 tmp_cost			= m_cost;
	bool result				= process_if_exists(section,	"cost",				tmp_cost,			test);
	if (result)
		m_upgrades_cost		+= tmp_cost - m_cost;
	result					|= process_if_exists(section,	"inv_weight",		m_weight,		test);
	result					|= process_if_exists(section,	"inv_volume",		m_volume,		test);

	bool result2			= false;
	if ( BaseSlot() != NO_ACTIVE_SLOT )
	{
		BOOL value			= m_flags.test(FAllowSprint);
		result2				= process_if_exists(section, "sprint_allowed", value, test);
		if (result2 && !test)
			m_flags.set(FAllowSprint, value);
		result				|= result2;

		float inertion						= m_fControlInertionFactor - 1.f;
		result2								|= process_if_exists(section, "control_inertion_factor", inertion, test);
		if (result2)
			m_fControlInertionFactor		= inertion + 1.f;
		result								|= result2;
	}

	LPCSTR								str;
	result2								= process_if_exists(section, "immunities_sect", str, test);
	if (result2 && !test)
		CHitImmunity::LoadImmunities	(str, pSettings);
	result2								= process_if_exists(section, "immunities_sect_add", str, test);
	if (result2 && !test)
		CHitImmunity::AddImmunities		(str, pSettings);

	result |= O.Aboba(eInstallUpgrade, (void*)section, (int)test) != flt_max;

	return result;
}
