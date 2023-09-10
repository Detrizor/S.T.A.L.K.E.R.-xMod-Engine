#include "pch_script.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../level.h"
#include "game_base_space.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "inventory_item_object.h"
#include "UIInventoryUtilities.h"
#include "WeaponMagazined.h"
#include "../string_table.h"
#include "UICellItem.h"
#include "addon_owner.h"
#include "ui\UICellCustomItems.h"

CUIWpnParams::CUIWpnParams()
{
	AttachChild		(&m_Prop_line);

	AttachChild		(&m_textBulletSpeed);
	AttachChild		(&m_textBulletSpeedValue);
	AttachChild		(&m_textRPM);
	AttachChild		(&m_textRPMValue);

	AttachChild		(&m_textAmmoTypes);
	AttachChild		(&m_textAmmoTypesValue);
	AttachChild		(&m_textAddonSlots);
	AttachChild		(&m_textAddonSlotsValue);
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
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_addon_slots",				0, &m_textAddonSlots);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:addon_slots_value",			0, &m_textAddonSlotsValue);
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
	shared_str& section					= itm->m_section;
	PIItem item							= (PIItem)itm->m_pData;
	CWeaponMagazined* wpn				= (item) ? item->cast<CWeaponMagazined*>() : NULL;

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

	CUIAddonOwnerCellItem* uiao			= smart_cast<CUIAddonOwnerCellItem*>(itm);
	if (uiao && uiao->Slots().size())
	{
		str								= "";
		for (auto slot : uiao->Slots())
		{
			if (str.size())
				str.printf				("%s, ", *str);
			str.printf					("%s%s", *str, *CStringTable().translate(slot->type));
		}
	}
	else
		str								= "---";
	m_textAddonSlotsValue.SetText		(*str);
	m_textAddonSlotsValue.AdjustHeightToText();
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