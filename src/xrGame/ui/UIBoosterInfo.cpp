#include "stdafx.h"
#include "UIBoosterInfo.h"
#include "UIStatic.h"
#include "object_broker.h"
#include "../EntityCondition.h"
#include "..\actor.h"
#include "../ActorCondition.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "../string_table.h"
#include "UICellItem.h"
#include "item_container.h"
#include "WeaponAmmo.h"
#include "Level_Bullet_Manager.h"
#include "BoneProtections.h"
#include "WeaponMagazined.h"

LPCSTR boost_influence_caption[] =
{
	"ui_inv_painkill",
	"ui_inv_regeneration",
	"ui_inv_recuperation",
	"ui_inv_endurance",
	"ui_inv_tenacity",
	"ui_inv_antirad"
};

void CUIBoosterInfo::InitFromXml(CUIXml& xml)
{
	LPCSTR base				= "booster_params";
	XML_NODE* stored_root	= xml.GetLocalRoot();
	XML_NODE* base_node		= xml.NavigateToNode( base, 0 );
	if(!base_node)
		return;

	CUIXmlInit::InitWindow	(xml, base, 0, this);
	xml.SetLocalRoot		(base_node);
	
	m_Prop_line						= xr_new<CUIStatic>();
	AttachChild						(m_Prop_line);
	CUIXmlInit::InitStatic			(xml, "prop_line", 0, m_Prop_line);

	for (u32 i = 0; i < eBoostMaxCount; ++i)
	{
		m_boosts[i]					= xr_new<UIBoosterInfoItem>();
		m_boosts[i]->Init			(xml, ef_boosters_section_names[i]);
		LPCSTR name					= *CStringTable().translate(boost_influence_caption[i]);
		m_boosts[i]->SetCaption		(name);
		xml.SetLocalRoot			(base_node);
	}

	m_need_hydration					= xr_new<UIBoosterInfoItem>();
	m_need_hydration->Init				(xml, "need_hydration");
	LPCSTR name							= CStringTable().translate("ui_inv_hydration").c_str();
	m_need_hydration->SetCaption		(name);
	xml.SetLocalRoot					(base_node);

	m_need_satiety						= xr_new<UIBoosterInfoItem>();
	m_need_satiety->Init				(xml, "need_satiety");
	name								= CStringTable().translate("ui_inv_satiety").c_str();
	m_need_satiety->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_health_outer						= xr_new<UIBoosterInfoItem>();
	m_health_outer->Init				(xml, "health_outer");
	name								= CStringTable().translate("ui_inv_health_outer").c_str();
	m_health_outer->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_health_neural						= xr_new<UIBoosterInfoItem>();
	m_health_neural->Init				(xml, "health_neural");
	name								= CStringTable().translate("ui_inv_health_neural").c_str();
	m_health_neural->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_power_short						= xr_new<UIBoosterInfoItem>();
	m_power_short->Init					(xml, "power_short");
	name								= CStringTable().translate("ui_inv_power_short").c_str();
	m_power_short->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_booster_anabiotic					= xr_new<UIBoosterInfoItem>();
	m_booster_anabiotic->Init			(xml, "anabiotic");
	name								= CStringTable().translate("ui_inv_survive_surge").c_str();
	m_booster_anabiotic->SetCaption		(name);
	xml.SetLocalRoot					(base_node);

	m_disclaimer						= xr_new<CUIStatic>();
	AttachChild							(m_disclaimer);
	CUIXmlInit::InitStatic				(xml, "ammo_disclaimer", 0, m_disclaimer);

	m_bullet_speed						= xr_new<UIBoosterInfoItem>();
	m_bullet_speed->Init				(xml, "bullet_speed");
	name								= CStringTable().translate("ui_bullet_speed").c_str();
	m_bullet_speed->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_bullet_pulse						= xr_new<UIBoosterInfoItem>();
	m_bullet_pulse->Init				(xml, "bullet_pulse");
	name								= CStringTable().translate("ui_bullet_pulse").c_str();
	m_bullet_pulse->SetCaption			(name);
	xml.SetLocalRoot					(base_node);

	m_armor_piercing					= xr_new<UIBoosterInfoItem>();
	m_armor_piercing->Init				(xml, "armor_piercing");
	xml.SetLocalRoot					(base_node);

	m_impair							= xr_new<UIBoosterInfoItem>();
	m_impair->Init						(xml, "impair");
	m_impair->SetCaption				(*CStringTable().translate("ui_impair"));
	xml.SetLocalRoot					(base_node);

	m_ammo_type							= xr_new<UIBoosterInfoItem>();
	m_ammo_type->Init					(xml, "ammo_type");
	name								= CStringTable().translate("ui_ammo_type").c_str();
	m_ammo_type->SetCaption				(name);
	xml.SetLocalRoot					(base_node);

	m_capacity							= xr_new<UIBoosterInfoItem>();
	m_capacity->Init					(xml, "capacity");
	name								= CStringTable().translate("ui_capacity").c_str();
	m_capacity->SetCaption				(name);
	xml.SetLocalRoot					(base_node);

	m_artefact_isolation				= xr_new<UIBoosterInfoItem>();
	m_artefact_isolation->Init			(xml, "artefact_isolation");
	name								= CStringTable().translate("ui_artefact_isolation").c_str();
	m_artefact_isolation->SetCaption	(name);
	m_artefact_isolation->SetStrValue	("");
	xml.SetLocalRoot					(base_node);

	m_radiation_protection				= xr_new<UIBoosterInfoItem>();
	m_radiation_protection->Init		(xml, "radiation_protection");
	name								= CStringTable().translate("ui_radiation_protection").c_str();
	m_radiation_protection->SetCaption	(name);
	xml.SetLocalRoot					(stored_root);
}

void CUIBoosterInfo::SetInfo	(CUICellItem* itm)
{
	DetachAll			();
	CActor* actor		= smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
		return;

	const shared_str&	section = itm->m_section;
	Fvector2			pos;
	float				val;
	AttachChild			(m_Prop_line);
	float h				= m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	for (u32 i = 0; i < eBoostMaxCount; ++i)
	{
		if (pSettings->line_exist(section, ef_boosters_section_names[i]))
		{
			val								= pSettings->r_float(section, ef_boosters_section_names[i]);
			if (fis_zero(val))
				continue;
			m_boosts[i]->SetValue			(val);
			pos.set							(m_boosts[i]->GetWndPos());
			pos.y							= h;
			m_boosts[i]->SetWndPos			(pos);
			h								+= m_boosts[i]->GetWndSize().y;
			AttachChild						(m_boosts[i]);
		}
	}

	if (pSettings->line_exist(section, "need_hydration"))
	{
		val									= pSettings->r_float(section, "need_hydration");
		if (!fis_zero(val))
		{
			m_need_hydration->SetValue		(val);
			pos.set							(m_need_hydration->GetWndPos());
			pos.y							= h;
			m_need_hydration->SetWndPos		(pos);
			h								+= m_need_hydration->GetWndSize().y;
			AttachChild						(m_need_hydration);
		}
	}

	if (pSettings->line_exist(section, "need_satiety"))
	{
		val									= pSettings->r_float(section, "need_satiety");
		if (!fis_zero(val))
		{
			m_need_satiety->SetValue		(val);
			pos.set							(m_need_satiety->GetWndPos());
			pos.y							= h;
			m_need_satiety->SetWndPos		(pos);
			h								+= m_need_satiety->GetWndSize().y;
			AttachChild						(m_need_satiety);
		}
	}

	if (pSettings->line_exist(section, "health_outer"))
	{
		val									= pSettings->r_float(section, "health_outer");
		if (!fis_zero(val))
		{
			m_health_outer->SetValue		(val);
			pos.set							(m_health_outer->GetWndPos());
			pos.y							= h;
			m_health_outer->SetWndPos		(pos);
			h								+= m_health_outer->GetWndSize().y;
			AttachChild						(m_health_outer);
		}
	}

	if (pSettings->line_exist(section, "health_neural"))
	{
		val									= pSettings->r_float(section, "health_neural");
		if (!fis_zero(val))
		{
			m_health_neural->SetValue		(val);
			pos.set							(m_health_neural->GetWndPos());
			pos.y							= h;
			m_health_neural->SetWndPos		(pos);
			h								+= m_health_neural->GetWndSize().y;
			AttachChild						(m_health_neural);
		}
	}

	if (pSettings->line_exist(section, "power_short"))
	{
		val									= pSettings->r_float(section, "power_short");
		if (!fis_zero(val))
		{
			m_power_short->SetValue			(val);
			pos.set							(m_power_short->GetWndPos());
			pos.y							= h;
			m_power_short->SetWndPos		(pos);
			h								+= m_power_short->GetWndSize().y;
			AttachChild						(m_power_short);
		}
	}

	if (!xr_strcmp(section, "drug_anabiotic"))
	{
		pos.set								(m_booster_anabiotic->GetWndPos());
		pos.y								= h;
		m_booster_anabiotic->SetWndPos		(pos);
		h									+= m_booster_anabiotic->GetWndSize().y;
		AttachChild							(m_booster_anabiotic);
	}

	if (ItemCategory(section, "ammo") && (ItemSubcategory(section, "box") || ItemSubcategory(section, "cartridge")))
	{
		LPCSTR cartridge_section		= (ItemSubcategory(section, "box")) ? pSettings->r_string(section, "supplies") : *section;
		auto cartridge					= CCartridge(cartridge_section);
		
		float barrel_len				= 0.f;
		if (auto ai = Actor()->inventory().ActiveItem())
			if (auto wpn = ai->cast<CWeaponMagazined*>())
				for (auto& type : wpn->m_ammoTypes)
					if (type == cartridge_section)
						barrel_len		= wpn->getBarrelLen();

		if (barrel_len)
			m_disclaimer->TextItemControl()->SetText(*CStringTable().translate("ui_disclaimer_active_weapon"));
		else
		{
			shared_str					str;
			str.printf					("%s %.0f %s:", *CStringTable().translate("ui_disclaimer_reference_weapon"), cartridge.param_s.barrel_length, *CStringTable().translate("st_mm"));
			m_disclaimer->TextItemControl()->SetText(*str);
			barrel_len					= cartridge.param_s.barrel_len;
		}
		float bullet_speed				= cartridge.param_s.bullet_speed_per_barrel_len * barrel_len;

		pos.set							(m_disclaimer->GetWndPos());
		pos.y							= h;
		m_disclaimer->SetWndPos			(pos);
		h								+= m_disclaimer->GetWndSize().y;
		AttachChild						(m_disclaimer);

		m_bullet_speed->SetValue		(bullet_speed);
		pos.set							(m_bullet_speed->GetWndPos());
		pos.y							= h;
		m_bullet_speed->SetWndPos		(pos);
		h								+= m_bullet_speed->GetWndSize().y;
		AttachChild						(m_bullet_speed);

		m_bullet_pulse->SetValue		(bullet_speed * cartridge.param_s.fBulletMass * cartridge.param_s.buckShot);
		pos.set							(m_bullet_pulse->GetWndPos());
		pos.y							= h;
		m_bullet_pulse->SetWndPos		(pos);
		h								+= m_bullet_pulse->GetWndSize().y;
		AttachChild						(m_bullet_pulse);

		float muzzle_energy				= cartridge.param_s.fBulletMass * _sqr(bullet_speed) * .5f;
		float muzzle_ap					= cartridge.param_s.bullet_k_ap * muzzle_energy / cartridge.param_s.fBulletResist;
		muzzle_ap						*= Level().BulletManager().m_fBulletGlobalAPScale;

		float level						= -1.f;
		for (int i = 0; i < SBoneProtections::s_armor_levels.size(); i++)
		{
			if (muzzle_ap < SBoneProtections::s_armor_levels[i])
			{
				if (i > 0)
				{
					float d_ap			= SBoneProtections::s_armor_levels[i] - SBoneProtections::s_armor_levels[i-1];
					float d_muz_ap		= muzzle_ap - SBoneProtections::s_armor_levels[i - 1];
					level				+= d_muz_ap / d_ap;
				}
				break;
			}
			else
				level					= (float)i;
		}

		if (level >= 0.f)
		{
			m_armor_piercing->SetCaption(*CStringTable().translate("ui_armor_piercing"));
			m_armor_piercing->SetValue	(floor(level * 10.f) * .1f);
		}
		else
		{
			m_armor_piercing->SetCaption(*CStringTable().translate("ui_armor_piercing_absent"));
			m_armor_piercing->SetStrValue("");
		}
		pos.set							(m_armor_piercing->GetWndPos());
		pos.y							= h;
		m_armor_piercing->SetWndPos		(pos);
		h								+= m_armor_piercing->GetWndSize().y;
		AttachChild						(m_armor_piercing);

		m_impair->SetValue				(cartridge.param_s.impair);
		pos.set							(m_impair->GetWndPos());
		pos.y							= h;
		m_impair->SetWndPos				(pos);
		h								+= m_impair->GetWndSize().y;
		AttachChild						(m_impair);
	}

	if (ItemCategory(section, "magazine"))
	{
		if (READ_IF_EXISTS(pSettings, r_bool, section, "can_be_discharged", TRUE))
		{
			LPCSTR ammo_class				= pSettings->r_string(section, "ammo_class");
			string128						buffer;
			LPCSTR ammo_type				= _GetItem(ammo_class, 0, buffer);
			LPCSTR ammo_type_name_s			= pSettings->r_string(ammo_type, "inv_name_short");
			m_ammo_type->SetStrValue		(*CStringTable().translate(ammo_type_name_s));
			pos.set							(m_ammo_type->GetWndPos());
			pos.y							= h;
			m_ammo_type->SetWndPos			(pos);
			h								+= m_ammo_type->GetWndSize().y;
			AttachChild						(m_ammo_type);
		}
		else
			m_ammo_type->SetStrValue		("");

		float capacity						= pSettings->r_float(section, "capacity");
		m_capacity->SetValue				(capacity);
		pos.set								(m_capacity->GetWndPos());
		pos.y								= h;
		m_capacity->SetWndPos				(pos);
		h									+= m_capacity->GetWndSize().y;
		AttachChild							(m_capacity);
	}

	auto item							= PIItem(itm->m_pData);
	auto cont							= (item) ? item->cast<CInventoryContainer*>() : nullptr;
	if ((cont || READ_IF_EXISTS(pSettings, r_bool, section, "container", FALSE)) && !pSettings->r_string(section, "supplies"))
	{
		float capacity					= (cont) ? cont->GetCapacity() : pSettings->r_float(section, "capacity");
		m_capacity->SetValue			(capacity);
		pos.set							(m_capacity->GetWndPos());
		pos.y							= h;
		m_capacity->SetWndPos			(pos);
		h								+= m_capacity->GetWndSize().y;
		AttachChild						(m_capacity);

		if ((cont) ? cont->ArtefactIsolation(true) : pSettings->r_bool(section, "artefact_isolation"))
		{
			pos.set						(m_artefact_isolation->GetWndPos());
			pos.y						= h;
			m_artefact_isolation->SetWndPos(pos);
			h							+= m_artefact_isolation->GetWndSize().y;
			AttachChild					(m_artefact_isolation);
		}

		float radiation_protection		= (cont) ? cont->RadiationProtection(true) : pSettings->r_float(section, "radiation_protection");
		if (fMore(radiation_protection, 0.f))
		{
			m_radiation_protection->SetValue(1.f - radiation_protection);
			pos.set						(m_radiation_protection->GetWndPos());
			pos.y						= h;
			m_radiation_protection->SetWndPos(pos);
			h							+= m_radiation_protection->GetWndSize().y;
			AttachChild					(m_radiation_protection);
		}
	}

	SetHeight							(h);
}

/// ----------------------------------------------------------------

UIBoosterInfoItem::UIBoosterInfoItem()
{
	m_caption				= NULL;
	m_value					= NULL;
	m_magnitude				= 1.0f;
	m_show_sign				= false;
	
	m_unit_str._set			("");
	m_texture_minus._set	("");
	m_texture_plus._set		("");
}

UIBoosterInfoItem::~UIBoosterInfoItem()
{
}

void UIBoosterInfoItem::Init(CUIXml& xml, LPCSTR section)
{
	CUIXmlInit::InitWindow		(xml, section, 0, this);
	xml.SetLocalRoot			(xml.NavigateToNode(section));

	m_caption			= UIHelper::CreateStatic(xml, "caption", this);
	m_value				= UIHelper::CreateTextWnd(xml, "value", this);
	m_magnitude			= xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
	m_show_sign			= (xml.ReadAttribInt("value", 0, "show_sign", 1) == 1);
	
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

void UIBoosterInfoItem::SetCaption(LPCSTR name)
{
	m_caption->TextItemControl()->SetText(name);
}

void UIBoosterInfoItem::SetValue(float value)
{
	value				*= m_magnitude;
	shared_str			str;
	bool format			= (abs(value - float((int)value)) < 0.05f);
	str.printf			((m_show_sign) ? ((format) ? "%+.0f" : "%+.1f") : ((format) ? "%.0f" : "%.1f"), value);
	if (m_perc_unit)
		str.printf("%s%%", *str);
	else if (m_unit_str.size())
		str.printf		("%s %s", *str, *m_unit_str);
	m_value->SetText	(*str);

	m_value->SetTextColor			(color_rgba(170, 170, 170, 255));
	if (m_texture_minus.size())
		m_caption->InitTexture		((value >= 0.f) ? *m_texture_plus : *m_texture_minus);
}

void UIBoosterInfoItem::SetStrValue(LPCSTR value)
{
	m_value->SetText(value);
}
