#include "stdafx.h"
#include "CUIAmmoInfo.h"

#include "UIStatic.h"
#include "UIXmlInit.h"
#include "UICellItem.h"
#include "CUIMiscInfoItem.h"

#include "Actor.h"
#include "WeaponAmmo.h"
#include "string_table.h"
#include "WeaponMagazined.h"
#include "BoneProtections.h"
#include "Level_Bullet_Manager.h"

CUIAmmoInfo::CUIAmmoInfo() = default;
CUIAmmoInfo::~CUIAmmoInfo() = default;

void CUIAmmoInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "ammo_info" };

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strBaseNode, 0));

	AttachChild(m_pDisclaimer.get());
	CUIXmlInit::InitStatic(xmlDoc, "disclaimer", 0, m_pDisclaimer.get());

	AttachChild(m_pBulletSpeed.get());
	m_pBulletSpeed->init(xmlDoc, "bullet_speed");
	m_pBulletSpeed->SetCaption(CStringTable().translate("ui_bullet_speed").c_str());

	AttachChild(m_pBulletPulse.get());
	m_pBulletPulse->init(xmlDoc, "bullet_pulse");
	m_pBulletPulse->SetCaption(CStringTable().translate("ui_bullet_pulse").c_str());

	AttachChild(m_pArmorPiercing.get());
	m_pArmorPiercing->init(xmlDoc, "armor_piercing");

	AttachChild(m_pImpair.get());
	m_pImpair->init(xmlDoc, "impair");
	m_pImpair->SetCaption(CStringTable().translate("ui_impair").c_str());

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIAmmoInfo::setInfo(CUICellItem* pCellItem)
{
	auto strCartridgeSection{ (ItemSubcategory(pCellItem->m_section, "box")) ? pSettings->r_string(pCellItem->m_section, "supplies") : pCellItem->m_section.c_str() };
	auto getBarrelLen = [strCartridgeSection]()
	{
		auto slot_type{ pSettings->r_string(strCartridgeSection, "slot_type") };
		if (auto ai{ Actor()->inventory().ActiveItem() })
			if (auto wpn{ ai->O.scast<CWeaponMagazined*>() })
				if (auto ao{ ai->O.mcast<MAddonOwner>() })
					for (auto const& s : ao->AddonSlots())
						if (CAddonSlot::isCompatible(s->type, slot_type))
							return wpn->getBarrelLen();
		return 0.F;
	};

	CCartridge cartridge{ strCartridgeSection };
	float fBarrelLen{ getBarrelLen()};

	if (fMore(fBarrelLen, 0.F))
		m_pDisclaimer->TextItemControl()->SetText(CStringTable().translate("ui_disclaimer_active_weapon").c_str());
	else
	{
		shared_str str;
		str.printf("%s %.0f %s:", CStringTable().translate("ui_disclaimer_reference_weapon").c_str(), cartridge.param_s.barrel_length, CStringTable().translate("st_mm").c_str());
		m_pDisclaimer->TextItemControl()->SetText(str.c_str());
		fBarrelLen = cartridge.param_s.barrel_len;
	}
	float h{ m_pDisclaimer->GetWndSize().y };

	float fBulletSpeed{ cartridge.param_s.bullet_speed_per_barrel_len * fBarrelLen };
	m_pBulletSpeed->SetValue(fBulletSpeed);
	m_pBulletSpeed->SetY(h);
	h += m_pBulletSpeed->GetWndSize().y;

	m_pBulletPulse->SetValue(fBulletSpeed * cartridge.param_s.fBulletMass * cartridge.param_s.buckShot);
	m_pBulletPulse->SetY(h);
	h += m_pBulletPulse->GetWndSize().y;

	float fMuzzleAP{ Level().BulletManager().calculateAP(cartridge.param_s.penetration, fBulletSpeed) };
	float fAPLevel{ flt_max };
	for (size_t i = 0; i < SBoneProtections::s_armor_levels.size(); ++i)
	{
		if (fMuzzleAP < SBoneProtections::s_armor_levels[i])
		{
			if (i > 0)
			{
				float fDAP{ SBoneProtections::s_armor_levels[i] - SBoneProtections::s_armor_levels[i - 1] };
				float fDMuzzleAP{ fMuzzleAP - SBoneProtections::s_armor_levels[i - 1] };
				fAPLevel += fDMuzzleAP / fDAP;
			}
			break;
		}
		else
			fAPLevel = static_cast<float>(i);
	}

	if (fAPLevel != flt_max)
	{
		m_pArmorPiercing->SetCaption(CStringTable().translate("ui_armor_piercing").c_str());
		m_pArmorPiercing->SetValue(floor(fAPLevel * 10.F) * .1F);
	}
	else
	{
		m_pArmorPiercing->SetCaption(CStringTable().translate("ui_armor_piercing_absent").c_str());
		m_pArmorPiercing->SetStrValue("");
	}
	m_pArmorPiercing->SetY(h);
	h += m_pArmorPiercing->GetWndSize().y;

	m_pImpair->SetValue(cartridge.param_s.impair);
	m_pImpair->SetY(h);
	h += m_pImpair->GetWndSize().y;

	SetHeight(h);
}
