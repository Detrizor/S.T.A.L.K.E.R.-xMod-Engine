
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
	//inherited::install_upgrade( section );
	bool result = CInventoryItemObjectOld::install_upgrade_impl(section, test);
	
	result |= install_upgrade_ammo_class( section, test );
	result |= install_upgrade_disp      ( section, test );
	result |= install_upgrade_hit       ( section, test );
	result |= install_upgrade_addon     ( section, test );
	return result;
}

bool CWeapon::install_upgrade_ammo_class( LPCSTR section, bool test )
{
	bool result							= process_if_exists(section, "ammo_mag_size", iMagazineSize, test);
	LPCSTR								str;
	bool result2						= process_if_exists(section, "ammo_class", str, test);
	if (result2 && !test)
	{
		m_ammoTypes.clear				();
		string128						ammoItem;
		int count						= _GetItemCount(str);
		for (int i = 0; i < count; ++i)
		{
			_GetItem					(str, i, ammoItem);
			m_ammoTypes.push_back		(ammoItem);
		}
		m_ammoType						= 0;
	}
	result								|= result2;

	return result;
}

bool CWeapon::install_upgrade_disp( LPCSTR section, bool test )
{
	bool result		= process_if_exists(section,	"fire_dispersion_condition_factor",		fireDispersionConditionFactor,		test);
	result			|= process_if_exists(section,	"fire_distance",						fireDistance,						test);

	u8 rm						= (cam_recoil.ReturnMode) ? 1 : 0;
	result						|= process_if_exists(section, "cam_return", rm, test);
	cam_recoil.ReturnMode		= (rm == 1);

	rm							= (cam_recoil.StopReturn) ? 1 : 0;
	result						|= process_if_exists(section, "cam_return_stop", rm, test);
	cam_recoil.StopReturn		= (rm == 1);

	result		|= process_if_exists_deg2rad(section,	"fire_dispersion_base",			fireDispersionBase,					test);

	result		|= process_if_exists_deg2rad(section,	"cam_relax_speed",				cam_recoil.RelaxSpeed,				test);
	result		|= process_if_exists_deg2rad(section,	"cam_relax_speed_ai",			cam_recoil.RelaxSpeed_AI,			test);
	result		|= process_if_exists_deg2rad(section,	"cam_dispersion",				cam_recoil.Dispersion,				test);
	result		|= process_if_exists_deg2rad(section,	"cam_dispersion_inc",			cam_recoil.DispersionInc,			test);

	result		|= process_if_exists		(section,	"cam_dispersion_frac",			cam_recoil.DispersionFrac,			test);

	result		|= process_if_exists_deg2rad(section,	"cam_max_angle",				cam_recoil.MaxAngleVert,			test);
	result		|= process_if_exists_deg2rad(section,	"cam_max_angle_horz",			cam_recoil.MaxAngleHorz,			test);
	result		|= process_if_exists_deg2rad(section,	"cam_step_angle_horz",			cam_recoil.StepAngleHorz,			test);

	VERIFY		(!fis_zero(cam_recoil.RelaxSpeed));
	VERIFY		(!fis_zero(cam_recoil.RelaxSpeed_AI));
	VERIFY		(!fis_zero(cam_recoil.MaxAngleVert));
	VERIFY		(!fis_zero(cam_recoil.MaxAngleHorz));

	result		|= process_if_exists_deg2rad(section,	"zoom_cam_relax_speed",			zoom_cam_recoil.RelaxSpeed,			test);
	result		|= process_if_exists_deg2rad(section,	"zoom_cam_relax_speed_ai",		zoom_cam_recoil.RelaxSpeed_AI,		test);
	result		|= process_if_exists_deg2rad(section,	"zoom_cam_dispersion",			zoom_cam_recoil.Dispersion,			test);
	result		|= process_if_exists_deg2rad(section,	"zoom_cam_dispersion_inc",		zoom_cam_recoil.DispersionInc,		test);

	result		|= process_if_exists		(section,	"zoom_cam_dispersion_frac",		zoom_cam_recoil.DispersionFrac,		test);

	result		|= process_if_exists_deg2rad(section,	"zoom_cam_max_angle",			zoom_cam_recoil.MaxAngleVert,		test);
	result		|= process_if_exists_deg2rad(section,	"zoom_cam_max_angle_horz",		zoom_cam_recoil.MaxAngleHorz,		test);
	result		|= process_if_exists_deg2rad(section,	"zoom_cam_step_angle_horz",		zoom_cam_recoil.StepAngleHorz,		test);

	VERIFY		(!fis_zero(zoom_cam_recoil.RelaxSpeed));
	VERIFY		(!fis_zero(zoom_cam_recoil.RelaxSpeed_AI));
	VERIFY		(!fis_zero(zoom_cam_recoil.MaxAngleVert));
	VERIFY		(!fis_zero(zoom_cam_recoil.MaxAngleHorz));
	
	result		|= process_if_exists		(section,	"crosshair_inertion",			m_crosshair_inertion,				test);

	result		|= process_if_exists		(section,	"PDM_disp_base",				m_pdm.m_fPDM_disp_base,				test);
	result		|= process_if_exists		(section,	"PDM_disp_vel_factor",			m_pdm.m_fPDM_disp_vel_factor,		test);
	result		|= process_if_exists		(section,	"PDM_disp_accel_factor",		m_pdm.m_fPDM_disp_accel_factor,		test);
	result		|= process_if_exists		(section,	"PDM_disp_crouch",				m_pdm.m_fPDM_disp_crouch,			test);
	result		|= process_if_exists		(section,	"PDM_disp_crouch_no_acc",		m_pdm.m_fPDM_disp_crouch_no_acc,	test);

	result		|= process_if_exists		(section,	"condition_shot_dec",			conditionDecreasePerShot,			test);
	result		|= process_if_exists		(section,	"condition_queue_shot_dec",		conditionDecreasePerQueueShot,		test);
	result		|= process_if_exists		(section,	"misfire_start_condition",		misfireStartCondition,				test);
	result		|= process_if_exists		(section,	"misfire_end_condition",		misfireEndCondition,				test);
	result		|= process_if_exists		(section,	"misfire_start_prob",			misfireStartProbability,			test);
	result		|= process_if_exists		(section,	"misfire_end_prob",				misfireEndProbability,				test);

	BOOL value							= m_zoom_params.m_bZoomEnabled;
	bool result2						= process_if_exists(section, "zoom_enabled", value, test);
	if (result2 && !test)
		m_zoom_params.m_bZoomEnabled	= !!value;
	result								|= result2;

	return result;
}

bool CWeapon::install_upgrade_hit( LPCSTR section, bool test )
{
	bool result = process_if_exists(section, "bullet_speed", m_fStartBulletSpeed, test);

	float rpm			= 60.0f / fOneShotTime;
	bool result2		= process_if_exists(section, "rpm", rpm, test);
	if (result2 && !test)
	{
		VERIFY			(rpm > 0.0f);
		fOneShotTime	= 60.0f / rpm;
	}
	result				|= result2;

	return result;
}


bool CWeapon::install_upgrade_addon( LPCSTR section, bool test )
{
	bool result									= false;
	int temp_int								= (int)m_eScopeStatus;
	bool result2								= process_if_exists(section, "scope_status", temp_int, test);
	if (result2 && !test)
	{
		m_eScopeStatus							= (ALife::EWeaponAddonStatus)temp_int;
		if (m_eScopeStatus == ALife::eAddonAttachable || m_eScopeStatus == ALife::eAddonPermanent)
		{
			result								|= process_if_exists(section, "holder_range_modifier", m_addon_holder_range_modifier, test);
			result								|= process_if_exists(section, "holder_fov_modifier", m_addon_holder_fov_modifier, test);

			shared_str							scope_sect;
			if (m_eScopeStatus == ALife::eAddonPermanent)
			{
				LPCSTR scope					= pSettings->r_string(section, "scope");
				scope_sect.printf				("%s_%s", section, scope);
				m_scopes.push_back				(scope);
				m_scopes_sect.push_back			(*scope_sect);
				m_cur_scope						= 0;
			}
			else
			{
				LPCSTR scopes					= pSettings->r_string(section, "scopes");
				for (int i = 0, count = _GetItemCount(scopes); i < count; ++i)
				{
					string128					scope;
					_GetItem					(scopes, i, scope);
					scope_sect.printf			("%s_%s", section, scope);
					m_scopes.push_back			(scope);
					m_scopes_sect.push_back		(*scope_sect);
				}
			}
		}
		InitAddons								();
	}
	result |= process_if_exists(section, "scope_zoom_factor", m_zoom_params.m_fScopeZoomFactor, test);
	result |= process_if_exists(section, "scope_min_zoom_factor", m_zoom_params.m_fScopeMinZoomFactor, test);
	result |= process_if_exists(section, "scope_nightvision", m_zoom_params.m_sUseZoomPostprocess, test);
	result |= process_if_exists(section, "scope_alive_detector", m_zoom_params.m_sUseBinocularVision, test);

	result |= result2;

	temp_int						= (int)m_eSilencerStatus;
	result2							= process_if_exists(section, "silencer_status", temp_int, test);
	if (result2 && !test)
	{
		m_eSilencerStatus			= (ALife::EWeaponAddonStatus)temp_int;
		if (m_eSilencerStatus != ALife::eAddonDisabled)
		{
			m_sSilencerName			= pSettings->r_string(section, "silencer_name");
			if (m_eSilencerStatus == ALife::eAddonPermanent)
				InitAddons			();
			else
			{
				m_iSilencerX		= pSettings->r_s32(section, "silencer_x");
				m_iSilencerY		= pSettings->r_s32(section, "silencer_y");
			}
		}
	}
	result							|= result2;

	temp_int							= (int)m_eGrenadeLauncherStatus;
	result2								= process_if_exists(section, "grenade_launcher_status", temp_int, test);
	if (result2 && !test)
	{
		m_eGrenadeLauncherStatus		= (ALife::EWeaponAddonStatus)temp_int;
		if (m_eGrenadeLauncherStatus != ALife::eAddonDisabled)
		{
			m_sGrenadeLauncherName		= pSettings->r_string(section, "grenade_launcher_name");
			if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
				InitAddons();
			else
			{
				m_iGrenadeLauncherX		= pSettings->r_s32(section, "grenade_launcher_x");
				m_iGrenadeLauncherY		= pSettings->r_s32(section, "grenade_launcher_y");
			}
		}
	}
	result								|= result2;

	return result;
}
