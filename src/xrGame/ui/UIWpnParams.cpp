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

	AttachChild		(&m_text_barrel_length);
	AttachChild		(&m_text_barrel_length_value);
	AttachChild		(&m_textRPM);
	AttachChild		(&m_textRPMValue);

	AttachChild		(&m_textAmmoTypes);
	AttachChild		(&m_textAmmoTypesValue);
}

void CUIWpnParams::InitFromXml(CUIXml& xml_doc)
{
	if (!xml_doc.NavigateToNode("wpn_params", 0))
		return;
	CUIXmlInit::InitWindow			(xml_doc, "wpn_params",								0, this);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:prop_line",					0, &m_Prop_line);

	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_barrel_length",			0, &m_text_barrel_length);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_barrel_length_value",		0, &m_text_barrel_length_value);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_rpm",						0, &m_textRPM);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_rpm_value",				0, &m_textRPMValue);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_ammo_types",				0, &m_textAmmoTypes);
	CUIXmlInit::InitTextWnd			(xml_doc, "wpn_params:cap_ammo_types_value",		0, &m_textAmmoTypesValue);
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

static float get_barrel_length(CUICellItem* itm)
{
	LPCSTR integrated_addons			= pSettings->r_string(itm->m_section, "integrated_addons");
	string64							tmp;
	for (int i = 0, cnt = _GetItemCount(integrated_addons); i < cnt; ++i)
	{
		_GetItem						(integrated_addons, i, tmp);
		if (strstr(tmp, "barrel"))
			return						pSettings->r_float(tmp, "length");
	}

	if (auto ao = smart_cast<CUIAddonOwnerCellItem*>(itm))
		for (auto& s : ao->Slots())
			if (strstr(*s->addon_section, "barrel"))
				return					pSettings->r_float(s->addon_section, "length");

	return								0.f;
}

void CUIWpnParams::SetInfo(CUICellItem* itm)
{
	PIItem item							= (PIItem)itm->m_pData;
	CWeaponMagazined* wpn				= (item) ? item->O.scast<CWeaponMagazined*>() : nullptr;

	float barrel_length					= (wpn) ? wpn->getBarrelLength() : get_barrel_length(itm);
	shared_str							str;
	str.printf							("%.0f %s", barrel_length, *CStringTable().translate("st_mm"));
	m_text_barrel_length_value.SetText	(*str);

	float rpm							= (wpn) ? wpn->GetRPM() : pSettings->r_float(itm->m_section, "rpm");
	str.printf							("%.0f %s", rpm, *CStringTable().translate("st_rpm"));
	m_textRPMValue.SetText				(*str);

	xr_vector<shared_str>				ammo_types;
	if (wpn)
		ammo_types						= wpn->m_ammoTypes;
	else
		FillVector						(ammo_types, *itm->m_section, "ammo_class");
	shared_str name_s					= CInventoryItem::readNameShort(ammo_types[0]);
	if (name_s.size())
	{
		str								= name_s;
		for (u32 i = 1, i_e = ammo_types.size(); i < i_e; i++)
		{
			name_s						= CInventoryItem::readNameShort(ammo_types[i]);
			str.printf					("%s, %s", str.c_str(), CStringTable().translate(name_s).c_str());
		}
	}
	else
		str								= "---";
	m_textAmmoTypesValue.SetText		(str.c_str());
}

// -------------------------------------------------------------------------------------------------

CUIConditionParams::CUIConditionParams()
{
	AttachChild( &m_progress );
	AttachChild( &m_text );
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
