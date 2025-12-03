
#include "stdafx.h"
#include "Weapon.h"

bool CWeapon::process_if_exists_deg2rad(LPCSTR section, LPCSTR name, float& value, bool test)
{
	if (!pSettings->line_exist(section, name))
		return			false;
	
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
		float base_value			= deg2rad(READ_IF_EXISTS(pSettings, r_float, m_section_id, name, 0.f));
		if (!xr_strcmp(process_type_l, "*+"))
			value					+= base_value * dvalue_l;
		else if (!xr_strcmp(process_type_l, "*-"))
			value					-= base_value * dvalue_l;
		else if (!xr_strcmp(process_type, "*"))
			value					*= dvalue;
		else if (!xr_strcmp(process_type, "+"))
			value					+= deg2rad(dvalue);
		else if (!xr_strcmp(process_type, "-"))
			value					-= deg2rad(dvalue);
		else
			value					= (float)deg2rad(atof(c_str));
	}

	return true;
}

bool CWeapon::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result							= inherited::install_upgrade_impl(section, test);

	result		|= process_if_exists_deg2rad(section,	"fire_dispersion_base",			fireDispersionBase,					test);
	result		|= process_if_exists		(section,	"condition_shot_dec",			conditionDecreasePerShot,			test);
	result		|= process_if_exists		(section,	"condition_queue_shot_dec",		conditionDecreasePerQueueShot,		test);

	return		result;
}
