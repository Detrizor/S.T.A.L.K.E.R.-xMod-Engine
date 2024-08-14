#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"

#include "..\actor.h"
#include "..\ActorCondition.h"
#include "object_broker.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "../string_table.h"
#include "../Inventory_Item.h"
#include "purchase_list.h"
#include "UICellItem.h"

#include "..\items_library.h"
#include "Artefact.h"

u32 const red_clr		= color_argb(255, 210, 50, 50);
u32 const green_clr		= color_argb(255, 170, 170, 170);

float m_max_restores[]		= { -1.f, -1.f, -1.f, -1.f };

CUIArtefactParams::CUIArtefactParams()
{
	for (u32 i = 0; i < eAbsorbationTypeMax; ++i)
		m_absorbation_item[i]	= NULL;
	for (u32 i = 0; i < eRestoreTypeMax; ++i)
		m_restore_item[i]		= NULL;
	m_drain_factor				= NULL;
	m_weight_dump				= NULL;
	m_armor						= NULL;
	m_radiation					= NULL;
}

CUIArtefactParams::~CUIArtefactParams()
{
	delete_data		(m_absorbation_item);
	delete_data		(m_restore_item);
	xr_delete		(m_drain_factor);
	xr_delete		(m_weight_dump);
	xr_delete		(m_armor);
	xr_delete		(m_radiation);
	xr_delete		(m_Prop_line);
}

LPCSTR af_absorbation_names[] = // EImmunityTypes
{
	"burn_absorbation",
	"shock_absorbation",
	"chemical_burn_absorbation",
	"radiation_absorbation",
	"telepatic_absorbation"
};

LPCSTR af_absorbation_captions[] =  // EImmunityTypes
{
	"ui_inv_outfit_burn_protection",
	"ui_inv_outfit_shock_protection",
	"ui_inv_outfit_chemical_burn_protection",
	"ui_inv_outfit_radiation_protection",
	"ui_inv_outfit_telepatic_protection",
};

LPCSTR af_restore_section_names[] = // EConditionRestoreTypes
{
	"radiation_speed",
	"painkill_speed",
	"regeneration_speed",
	"recuperation_speed"
};

LPCSTR af_restore_caption[] =  // EConditionRestoreTypes
{
	"ui_inv_radiation",
	"ui_inv_painkill",
	"ui_inv_regeneration",
	"ui_inv_recuperation"
};

void CUIArtefactParams::InitFromXml( CUIXml& xml )
{
	LPCSTR base					= "af_params";
	XML_NODE* stored_root		= xml.GetLocalRoot();
	XML_NODE* base_node			= xml.NavigateToNode(base, 0);
	if (!base_node)
		return;
	CUIXmlInit::InitWindow		(xml, base, 0, this);
	xml.SetLocalRoot			(base_node);

	m_Prop_line						= xr_new<CUIStatic>();
	AttachChild						(m_Prop_line);
	m_Prop_line->SetAutoDelete		(false);
	CUIXmlInit::InitStatic			(xml, "prop_line", 0, m_Prop_line);
	
	for (u32 i = 0; i < eAbsorbationTypeMax; ++i)
	{
		m_absorbation_item[i]				= xr_new<UIArtefactParamItem>();
		m_absorbation_item[i]->Init			(xml, af_absorbation_names[i]);
		m_absorbation_item[i]->SetAutoDelete(false);
		LPCSTR name							= *CStringTable().translate(af_absorbation_captions[i]);
		m_absorbation_item[i]->SetCaption	(name);
		xml.SetLocalRoot					(base_node);
	}

	for (u32 i = 0; i < eRestoreTypeMax; ++i)
	{
		m_restore_item[i]				= xr_new<UIArtefactParamItem>();
		m_restore_item[i]->Init			(xml, af_restore_section_names[i]);
		m_restore_item[i]->SetAutoDelete(false);
		LPCSTR name						= *CStringTable().translate(af_restore_caption[i]);
		m_restore_item[i]->SetCaption	(name);
		xml.SetLocalRoot				(base_node);
	}
	
	m_drain_factor						= xr_new<UIArtefactParamItem>();
	m_drain_factor->Init				(xml, "drain_factor");
	m_drain_factor->SetAutoDelete		(false);
	m_drain_factor->SetCaption			(*CStringTable().translate("st_drain_factor"));
	xml.SetLocalRoot					(base_node);
	
	m_weight_dump						= xr_new<UIArtefactParamItem>();
	m_weight_dump->Init					(xml, "weight_dump");
	m_weight_dump->SetAutoDelete		(false);
	m_weight_dump->SetCaption			(*CStringTable().translate("st_weight_dump"));
	xml.SetLocalRoot					(base_node);
	
	m_armor								= xr_new<UIArtefactParamItem>();
	m_armor->Init						(xml, "armor");
	m_armor->SetAutoDelete				(false);
	m_armor->SetCaption					(*CStringTable().translate("st_armor"));
	xml.SetLocalRoot					(base_node);

	m_radiation							= xr_new<UIArtefactParamItem>();
	m_radiation->Init					(xml, "radiation");
	m_radiation->SetAutoDelete			(false);
	m_radiation->SetCaption				(*CStringTable().translate("st_radiation"));

	xml.SetLocalRoot					(stored_root);
}

void InitMaxArtValues()
{
	for (auto& sec : CItemsLibrary::getDivision("artefact", "active", "void"))
	{
		for (u8 i = 0; i < eRestoreTypeMax; ++i)
		{
			float val					= pSettings->r_float(sec, af_restore_section_names[i]);
			if (val > m_max_restores[i])
				m_max_restores[i]		= val;
		}
	}
}

void CUIArtefactParams::SetInfo(LPCSTR section, CArtefact* art)
{
	DetachAll							();
	CActor* actor						= smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
		return;

	Fvector2							pos;
	float								val, h;
	if (xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "description", ""), ""))
	{
		AttachChild						(m_Prop_line);
		h								= m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;
	}
	else
		h								= 0.f;

	for (int i = 0; i < eAbsorbationTypeMax; ++i)
	{
		val								= (art) ? art->Absorbation(i) : pSettings->r_float(section, af_absorbation_names[i]);
		if (!fis_zero(val))
			SetInfoItem					(m_absorbation_item[i], val, pos, h);
	}

	for (u32 i = 0; i < eRestoreTypeMax; ++i)
	{
		val								= pSettings->r_float(section, af_restore_section_names[i]) * ((art) ? art->Power() : 1.f);
		if (fis_zero(val))
			continue;

		if (m_max_restores[i] == -1.f)
			InitMaxArtValues			();
		if (i != eRadiationSpeed)
			val							/= m_max_restores[i];

		SetInfoItem						(m_restore_item[i], val, pos, h);
	}

	val									= (art) ? art->DrainFactor() : pSettings->r_float(section, "drain_factor");
	if (!fis_zero(val))
		SetInfoItem						(m_drain_factor, val, pos, h);

	val									= (art) ? art->WeightDump() : pSettings->r_float(section, "weight_dump");
	if (!fis_zero(val))
		SetInfoItem						(m_weight_dump, val, pos, h);

	val									= (art) ? art->GetArmor() : pSettings->r_float(section, "armor");
	if (!fis_zero(val))
		SetInfoItem						(m_armor, val, pos, h);

	val									= (art) ? art->Radiation() : pSettings->r_float(section, "radiation");
	if (!fis_zero(val))
		SetInfoItem						(m_radiation, val, pos, h);

	SetHeight							(h);
}

void CUIArtefactParams::SetInfoItem(UIArtefactParamItem* item, float value, Fvector2& pos, float& h)
{
	item->SetValue						(value);
	pos.set								(item->GetWndPos());
	pos.y								= h;
	item->SetWndPos						(pos);
	h									+= item->GetWndSize().y;
	AttachChild							(item);
}

/// ----------------------------------------------------------------

UIArtefactParamItem::UIArtefactParamItem()
{
	m_caption   = NULL;
	m_value     = NULL;
	m_magnitude = 1.0f;
	m_sign_inverse = false;
	
	m_unit_str._set( "" );
	m_texture_minus._set( "" );
	m_texture_plus._set( "" );
}

UIArtefactParamItem::~UIArtefactParamItem()
{
}

void UIArtefactParamItem::Init(CUIXml& xml, LPCSTR section)
{
	CUIXmlInit::InitWindow		(xml, section, 0, this);
	xml.SetLocalRoot			(xml.NavigateToNode(section));

	m_caption			= UIHelper::CreateStatic(xml, "caption", this);
	m_value				= UIHelper::CreateTextWnd(xml, "value", this);
	m_magnitude			= xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
	m_sign_inverse		= (xml.ReadAttribInt("value", 0, "sign_inverse", 0) == 1);
	
	m_perc_unit			= (!!xml.ReadAttribInt("value", 0, "perc_unit", 0));
	LPCSTR unit_str		= xml.ReadAttrib("value", 0, "unit_str", "");
	m_unit_str._set		(CStringTable().translate(unit_str));

	LPCSTR texture_minus		= xml.Read("texture_minus", 0, "");
	if (texture_minus && xr_strlen(texture_minus))
	{
		m_texture_minus._set	(texture_minus);
		LPCSTR texture_plus		= xml.Read("caption:texture", 0, "");
		m_texture_plus._set		(texture_plus);
		VERIFY					(m_texture_plus.size());
	}
}

void UIArtefactParamItem::SetCaption(LPCSTR name)
{
	m_caption->TextItemControl()->SetText(name);
}

void UIArtefactParamItem::SetValue(float value)
{
	value				*= m_magnitude;
	shared_str			str;
	str.printf			((m_perc_unit) ? "%.0f%%" : ((abs(value - float((int)value)) < 0.05f) ? "%.0f" : "%.1f"), value);
	if (m_unit_str.size())
		str.printf		("%s %s", *str, *m_unit_str);
	m_value->SetText	(*str);

	bool positive					= (value >= 0.0f);
	if (m_sign_inverse)
		positive					= !positive;
	u32 color						= (positive) ? green_clr : red_clr;
	m_value->SetTextColor			(color);
	if (m_texture_minus.size())
		m_caption->InitTexture		((positive) ? *m_texture_plus : *m_texture_minus);
}
