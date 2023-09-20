#include "pch_script.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../level.h"
#include "game_base_space.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "inventory_item_object.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"
#include "../string_table.h"
#include "UICellItem.h"

CUIWpnParams::CUIWpnParams()
{
	AttachChild		(&m_Prop_line);

	AttachChild		(&m_textBulletSpeed);
	AttachChild		(&m_textBulletSpeedValue);
	AttachChild		(&m_textRPM);
	AttachChild		(&m_textRPMValue);

	AttachChild		(&m_textAmmoTypes);
	AttachChild		(&m_textAmmoTypesValue);
	AttachChild		(&m_textMagazineTypes);
	AttachChild		(&m_textMagazineTypesValue);
	AttachChild		(&m_textScopes);
	AttachChild		(&m_textScopesValue);
	AttachChild		(&m_textSilencer);
	AttachChild		(&m_textSilencerValue);
	AttachChild		(&m_textGLauncher);
	AttachChild		(&m_textGLauncherValue);
}

CUIWpnParams::~CUIWpnParams()
{
}

void CUIWpnParams::InitFromXml(CUIXml& xml_doc)
{
	if (!xml_doc.NavigateToNode("wpn_params", 0))
		return;
	CUIXmlInit::InitWindow			(xml_doc, "wpn_params",								0, this);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:prop_line",					0, &m_Prop_line);

	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_bullet_speed",			0, &m_textBulletSpeed);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_bullet_speed_value",		0, &m_textBulletSpeedValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_rpm",						0, &m_textRPM);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_rpm_value",				0, &m_textRPMValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_ammo_types",				0, &m_textAmmoTypes);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_ammo_types_value",		0, &m_textAmmoTypesValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_magazine_types",			0, &m_textMagazineTypes);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_magazine_types_value",	0, &m_textMagazineTypesValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_scopes",					0, &m_textScopes);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_scopes_value",			0, &m_textScopesValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_silencer",				0, &m_textSilencer);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_silencer_value",			0, &m_textSilencerValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_glauncher",				0, &m_textGLauncher);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_glauncher_value",			0, &m_textGLauncherValue);
}

void FillVector(xr_vector<shared_str>& vector, LPCSTR section, LPCSTR line)
{
	LPCSTR str					= READ_IF_EXISTS(pSettings, r_string, section, line, 0);
	if (str && str[0])
	{
		string128				tmp;
		for (int it = 0, count = _GetItemCount(str); it < count; ++it)
		{
			_GetItem			(str, it, tmp);
			vector.push_back	(tmp);
		}
	}
}

void CUIWpnParams::SetInfo(CUICellItem* itm)
{
	shared_str& section			= itm->m_section;
	PIItem item					= (PIItem)itm->m_pData;
	CWeapon* wpn				= (item) ? item->cast_weapon() : NULL;

	float bullet_speed					= (wpn) ? wpn->GetBulletSpeed() : pSettings->r_float(section, "bullet_speed");
	float rpm							= (wpn) ? wpn->GetRPM() : pSettings->r_float(section, "rpm");
	shared_str							str;
	LPCSTR mps_str						= CStringTable().translate("st_mps").c_str();
	str.printf							("%.0f %s", bullet_speed, mps_str);
	m_textBulletSpeedValue.SetText		(*str);
	LPCSTR rpm_str						= CStringTable().translate("st_rpm").c_str();
	str.printf							("%.0f %s", rpm, rpm_str);
	m_textRPMValue.SetText				(*str);

	xr_vector<shared_str>				ammo_types;
	if (wpn)
		ammo_types						= wpn->m_ammoTypes;
	else
		FillVector						(ammo_types, *section, "ammo_class");
	LPCSTR name_s						= READ_IF_EXISTS(pSettings, r_string, ammo_types[0], "inv_name_short", false);
	if (name_s)
	{
		str._set						(CStringTable().translate(name_s));
		for (u32 i = 1, i_e = ammo_types.size(); i < i_e; i++)
		{
			name_s						= pSettings->r_string(ammo_types[i], "inv_name_short");
			str.printf					("%s, %s", *str, *CStringTable().translate(name_s));
		}
	}
	else
		str._set						("---");
	m_textAmmoTypesValue.SetText		(*str);

	xr_vector<shared_str>				mag_types;
	if (wpn)
		mag_types						= wpn->m_magazineTypes;
	else
		FillVector						(mag_types, *section, "magazine_class");
	if (mag_types.size())
	{
		LPCSTR name_s					= pSettings->r_string(mag_types[0], "inv_name_short");
		str._set						(CStringTable().translate(name_s));
		for (u32 i = 1, count = mag_types.size(); i < count; i++)
		{
			LPCSTR mag_section			= *mag_types[i];
			if (READ_IF_EXISTS(pSettings, r_bool, mag_section, "pseudosection", FALSE))
				continue;
			name_s						= pSettings->r_string(mag_section, "inv_name_short");
			str.printf					("%s, %s", *str, *CStringTable().translate(name_s));
		}
	}
	else
		str._set						("---");
	m_textMagazineTypesValue.SetText	(*str);

	u8 scope_status						= (wpn) ? (u8)wpn->get_ScopeStatus() : pSettings->r_u8(section, "scope_status");
	if (scope_status == 2)
	{
		xr_vector<shared_str>			scopes;
		if (wpn)
			scopes						= wpn->m_scopes;
		else
			FillVector					(scopes, *section, "scopes");
		LPCSTR name						= pSettings->r_string(scopes[0], "inv_name_short");
		str._set						(CStringTable().translate(name));
		for (u32 i = 1, count = scopes.size(); i < count; i++)
		{
			name						= pSettings->r_string(scopes[i], "inv_name_short");
			str.printf					("%s, %s", *str, *CStringTable().translate(name));
		}
		m_textScopes.SetText			(*CStringTable().translate("st_scopes"));
	}
	else if (scope_status == 1)
	{
		LPCSTR scope					= (wpn) ? *wpn->GetScopeName() : READ_IF_EXISTS(pSettings, r_string, section, "scope", 0);
		LPCSTR name						= pSettings->r_string(scope, "inv_name_short");
		str.printf						("%s (%s)", *CStringTable().translate(name), *CStringTable().translate("st_integraged"));
		m_textScopes.SetText			(*CStringTable().translate("st_scope"));
	}
	else
		str._set						("---");
	m_textScopesValue.SetText			(*str);

	u8 silencer_status					= (wpn) ? (u8)wpn->get_SilencerStatus() : pSettings->r_u8(section, "silencer_status");
	if (silencer_status)
	{
		LPCSTR silencer					= (wpn) ? *wpn->GetSilencerName() : READ_IF_EXISTS(pSettings, r_string, section, "silencer_name", 0);
		LPCSTR name						= pSettings->r_string(silencer, "inv_name_short");
		if (silencer_status == 2)
			str._set					(CStringTable().translate(name));
		else
			str.printf					("%s (%s)", *CStringTable().translate(name), *CStringTable().translate("st_integraged"));
	}
	else
		str._set						("---");
	m_textSilencerValue.SetText			(*str);
	
	u8 glauncher_status					= (wpn) ? (u8)wpn->get_GrenadeLauncherStatus() : pSettings->r_u8(section, "grenade_launcher_status");
	if (glauncher_status)
	{
		LPCSTR glauncher				= (wpn) ? *wpn->GetGrenadeLauncherName() : READ_IF_EXISTS(pSettings, r_string, section, "grenade_launcher_name", 0);
		LPCSTR name						= pSettings->r_string(glauncher, "inv_name_short");
		if (glauncher_status == 2)
			str._set					(CStringTable().translate(name));
		else
			str.printf					("%s (%s)", *CStringTable().translate(name), *CStringTable().translate("st_integraged"));
	}
	else
		str._set						("---");
	m_textGLauncherValue.SetText		(*str);
}

// -------------------------------------------------------------------------------------------------

CUIConditionParams::CUIConditionParams()
{
	AttachChild( &m_progress );
	AttachChild( &m_text );
}

CUIConditionParams::~CUIConditionParams()
{
}

void CUIConditionParams::InitFromXml(CUIXml& xml_doc)
{
	if (!xml_doc.NavigateToNode("condition_params", 0))
		return;
	CUIXmlInit::InitWindow		(xml_doc, "condition_params", 0, this);
	CUIXmlInit::InitStatic		(xml_doc, "condition_params:caption", 0, &m_text);
	m_progress.InitFromXml		(xml_doc, "condition_params:progress_state");
}

void CUIConditionParams::SetInfo(CInventoryItem const* slot_item, CInventoryItem const& cur_item )
{
	float cur_value		= cur_item.GetConditionToShow() * 100.0f + 1.0f - EPS;
	float slot_value	= cur_value;
	if (slot_item && (slot_item != &cur_item))
		slot_value		= slot_item->GetConditionToShow() * 100.0f + 1.0f - EPS;
	m_progress.SetTwoPos( cur_value, slot_value );
}