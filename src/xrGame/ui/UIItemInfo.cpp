#include "pch_script.h"

#include "uistatic.h"
#include "UIXmlInit.h"
#include "uiiteminfo.h"

#include "UIScrollView.h"
#include "UIFrameWindow.h"
#include "UIProgressBar.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "UIInventoryUtilities.h"
#include "UIWpnParams.h"
#include "CUIArtefactParams.h"
#include "UIInvUpgradeProperty.h"
#include "UIOutfitInfo.h"
#include "UIBoosterInfo.h"
#include "UICellItem.h"
#include "UIActorMenu.h"
#include "uigamecustom.h"
#include "clsid_game.h"
#include "CUIAddonInfo.h"
#include "CUIAmmoInfo.h"
#include "CUIMiscInfo.h"
#include "UICellCustomItems.h"

#include "../Weapon.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../eatable_item.h"
#include "../string_table.h"
#include "../Inventory_Item.h"
#include "../PhysicsShellHolder.h"

namespace
{
	string256 buffer;

	class CTierChecker
	{
		struct STier
		{
			float fTreshold{};
			LPCSTR strTitle{};
			bool bInclusive{ true };
		};

	public:
		CTierChecker(std::initializer_list<STier> tiers) : m_tiers(tiers) {}

	public:
		LPCSTR getTier(float fValue) const
		{
			for (auto const& tier : m_tiers)
				if (((tier.bInclusive) ? fMoreOrEqual : fMore)(fValue, tier.fTreshold, EPS_S))
					return tier.strTitle;
			return m_tiers.back().strTitle;
		}

	private:
		const std::vector<STier> m_tiers;
	};

	const CTierChecker conditionTiers{
		{ 1.F, "st_prestine" },
		{ .8F, "st_almost_prestine" },
		{ .6F, "st_worn" },
		{ .4F, "st_damaged" },
		{ .2F, "st_badly_damaged" },
		{ 0.F, "st_almost_ruined", false },
		{ 0.F, "st_ruined" }
	};

	const CTierChecker fillTiers{
		{ 1.F, "st_full" },
		{ .8F, "st_almost_full" },
		{ .6F, "st_more_than_half" },
		{ .4F, "st_around_half" },
		{ .2F, "st_less_than_half" },
		{ 0.F, "st_almost_empty", false },
		{ 0.F, "st_empty" }
	};
}

extern const LPCSTR g_inventory_upgrade_xml;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CUIItemInfo::CUIItemInfo() = default;
CUIItemInfo::~CUIItemInfo() = default;

void CUIItemInfo::init(Fvector2 pos, Fvector2 size, LPCSTR xml_name)
{
	_super::SetWndPos(pos);
	_super::SetWndSize(size);
	init(xml_name);
}

void CUIItemInfo::init(LPCSTR strXmlName)
{
	CUIXml uiXml{};
	uiXml.Load(CONFIG_PATH, UI_PATH, strXmlName);

	CUIXmlInit::InitWindow(uiXml, "main_frame", 0, this);

	AttachChild(m_pFrame.get());
	CUIXmlInit::InitFrameWindow(uiXml, "background_frame", 0, m_pFrame.get());

	AttachChild(m_pBackground.get());
	CUIXmlInit::InitStatic(uiXml, "background", 0, m_pBackground.get());

	AttachChild(m_pUIName.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_name", 0, m_pUIName.get());

	AttachChild(m_pUIWeight.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_weight", 0, m_pUIWeight.get());

	AttachChild(m_pUIVolume.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_volume", 0, m_pUIVolume.get());

	AttachChild(m_pUICondition.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_condition", 0, m_pUICondition.get());

	AttachChild(m_pUIAmount.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_amount", 0, m_pUIAmount.get());

	AttachChild(m_pUICost.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_cost", 0, m_pUICost.get());

	AttachChild(m_pUITradeTip.get());
	CUIXmlInit::InitTextWnd(uiXml, "static_no_trade", 0, m_pUITradeTip.get());

	m_pUIOutfitInfo->InitFromXml(uiXml);
	m_pUIWpnParams->InitFromXml(uiXml);
	m_pUIArtefactParams->initFromXml(uiXml);
	m_pUIAddonInfo->initFromXml(uiXml);
	m_pUIAmmoInfo->initFromXml(uiXml);
	m_pUIBoosterInfo->initFromXml(uiXml);
	m_pUIMiscInfo->initFromXml(uiXml);
	m_pUIInvUpgProperties->initFromXml(uiXml);

	AttachChild(m_pUIDesc.get());
	CUIXmlInit::InitScrollView(uiXml, "descr_list", 0, m_pUIDesc.get());
	if (!!uiXml.ReadAttribInt("descr_list", 0, "only_text_info", 1))
		m_descInfo.text.construct();
	m_descInfo.bFitToHeight = !!uiXml.ReadAttribInt("descr_list", 0, "fit_to_height", 0);
	CUIXmlInit::InitFont(uiXml, "descr_list:font", 0, m_descInfo.uDescClr, m_descInfo.pDescFont);

	if (uiXml.NavigateToNode("image_static", 0))
	{	
		m_pUIItemImage.construct();
		CUIXmlInit::InitStatic(uiXml, "image_static", 0, m_pUIItemImage.get());
		m_pUIItemImage->TextureOn();
		m_pUIItemImage->TextureOff();
		m_UIItemImageSize.set(m_pUIItemImage->GetWidth(),m_pUIItemImage->GetHeight());
	}

	CUIXmlInit::InitAutoStaticGroup(uiXml, "auto", 0, this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CUIItemInfo::set_amount_info(CUICellItem* pCellItem, CInventoryItem* pItem)
{
	auto strAmountDisplayType{ pSettings->r_string(pCellItem->m_section, "amount_display_type") };
	if (!strcmp(strAmountDisplayType, "hide"))
	{
		m_pUIAmount->SetText("");
		return;
	}

	auto strTitle{ CStringTable().translate(pSettings->r_string(strAmountDisplayType, "title")) };
	auto strUnit{ pSettings->r_string(strAmountDisplayType, "unit") };
	const bool bEmptyCont{ pSettings->r_bool_ex(pCellItem->m_section, "container", false) && !pSettings->r_u16(pCellItem->m_section, "supplies_count") };
	const float fFill{ (pItem) ? pItem->GetFill() : (bEmptyCont) ? 0.F : 1.F };

	if (!strcmp(strUnit, "percent"))
		xr_sprintf(buffer, "%s: %i%%", strTitle.c_str(), iFloor(fFill * 100.F));
	else if (!strcmp(strUnit, "fill"))
	{
		const auto strFill{ fillTiers.getTier(fFill) };
		xr_sprintf(buffer, "%s: %s", strTitle.c_str(), CStringTable().translate(strFill).c_str());
	}
	else
	{
		xr_sprintf(buffer, "%s:", strTitle.c_str());

		float fAmount{ (pItem) ? pItem->GetAmount() : ((bEmptyCont) ? 0.F : pSettings->r_float(pCellItem->m_section, "max_amount")) };
		fAmount *= pSettings->r_float(strAmountDisplayType, "factor");
		bool bInteger{ pSettings->r_bool(strAmountDisplayType, "integer") };

		size_t len{ strlen(buffer) };
		if (bInteger)
			snprintf(buffer + len, sizeof(buffer) - len, " %d", static_cast<int>(round(fAmount)));
		else
			snprintf(buffer + len, sizeof(buffer) - len, " %.2f", fAmount);

		if (pSettings->r_bool(strAmountDisplayType, "show_max"))
		{
			const float fMaxAmount{ (pItem) ? pItem->O.mcast<MAmountable>()->getMaxAmount() : pSettings->r_float(pCellItem->m_section, "max_amount") };
			len = strlen(buffer);
			if (bInteger)
				snprintf(buffer + len, sizeof(buffer) - len, "/%d", static_cast<int>(round(fMaxAmount)));
			else
				snprintf(buffer + len, sizeof(buffer) - len, "/%.2f", fMaxAmount);
		}

		if (!!strcmp(strUnit, "none"))
		{
			len = strlen(buffer);
			snprintf(buffer + len, sizeof(buffer) - len, " %s", CStringTable().translate(strUnit).c_str());
		}
	}

	m_pUIAmount->SetText(buffer);
}

void CUIItemInfo::set_description_info(CUICellItem* pCellItem, CInventoryItem* pItem, bool bTradeTip)
{
	auto pos{ m_pUIDesc->GetWndPos() };
	if (bTradeTip)
		pos.y = m_pUITradeTip->GetWndPos().y + m_pUITradeTip->GetHeight() + 4.F;

	m_pUIDesc->SetWndPos(pos);
	m_pUIDesc->Clear();
	VERIFY(!m_pUIDesc->GetSize());

	if (m_descInfo.text)
	{
		m_descInfo.text->SetTextColor(m_descInfo.uDescClr);
		m_descInfo.text->SetFont(m_descInfo.pDescFont);
		m_descInfo.text->SetWidth(m_pUIDesc->GetDesiredChildWidth());
		m_descInfo.text->SetTextComplexMode(true);
		m_descInfo.text->SetText((pItem) ? pItem->ItemDescription() : CInventoryItem::readDescription(pCellItem->m_section.c_str()));
		m_descInfo.text->AdjustHeightToText();
		m_pUIDesc->AddWindow(m_descInfo.text.get(), false);
	}

	set_custom_info(pCellItem, pItem);

	if (m_descInfo.bFitToHeight)
	{
		m_pUIDesc->SetHeight(m_pUIDesc->GetPadSize().y);
		float fNewHeight{ m_pUIDesc->GetWndPos().y + m_pUIDesc->GetWndSize().y + m_pUIName->GetWndPos().y };
		SetHeight(fNewHeight);
		m_pFrame->SetHeight(fNewHeight);
		m_pBackground->SetHeight(fNewHeight);
	}

	m_pUIDesc->ScrollToBegin();
}

void CUIItemInfo::set_custom_info(CUICellItem* pCellItem, CInventoryItem* pItem)
{

	if (ItemCategory(pCellItem->m_section, "outfit"))
	{
		if (ItemSubcategory(pCellItem->m_section, "helmet"))
			m_pUIOutfitInfo->setInfoHelmet(pCellItem);
		else
			m_pUIOutfitInfo->setInfoSuit(pCellItem);
		m_pUIDesc->AddWindow(m_pUIOutfitInfo.get(), false);
	}

	if (ItemCategory(pCellItem->m_section, "weapon") && !ItemSubcategory(pCellItem->m_section, "melee") && !ItemSubcategory(pCellItem->m_section, "grenade"))
	{
		m_pUIWpnParams->SetInfo(pCellItem);
		m_pUIDesc->AddWindow(m_pUIWpnParams.get(), false);
	}

	if (ItemCategory(pCellItem->m_section, "artefact"))
	{
		m_pUIArtefactParams->setInfo(smart_cast<CArtefact*>(pCellItem->getItem()), pCellItem->m_section.c_str());
		m_pUIDesc->AddWindow(m_pUIArtefactParams.get(), false);
	}

	if (pSettings->r_bool_ex(pCellItem->m_section, "addon", false) || smart_cast<CUIAddonOwnerCellItem*>(pCellItem))
	{
		m_pUIAddonInfo->setInfo(pCellItem);
		m_pUIDesc->AddWindow(m_pUIAddonInfo.get(), false);
	}

	if (ItemCategory(pCellItem->m_section, "ammo") && (ItemSubcategory(pCellItem->m_section, "box") || ItemSubcategory(pCellItem->m_section, "cartridge")))
	{
		m_pUIAmmoInfo->setInfo(pCellItem);
		m_pUIDesc->AddWindow(m_pUIAmmoInfo.get(), false);
	}

	m_pUIBoosterInfo->setInfo(pCellItem);
	m_pUIDesc->AddWindow(m_pUIBoosterInfo.get(), false);

	m_pUIMiscInfo->setInfo(pCellItem);
	m_pUIDesc->AddWindow(m_pUIMiscInfo.get(), false);

	if (pItem && pItem->upgardes().size())
	{
		m_pUIInvUpgProperties->setInfo(pItem);
		m_pUIDesc->AddWindow(m_pUIInvUpgProperties.get(), false);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CUIItemInfo::setItem(CUICellItem* pCellItem, u32 nItemPrice, LPCSTR strTradeTip)
{
	m_pCurItem = pCellItem;
	if (!pCellItem)
	{
		Enable(false);
		return;
	}

	auto pItem{ pCellItem->getItem() };
	auto const& strSection{ pCellItem->m_section };
	Enable(pItem || strSection.size() && strSection[0]);
	if (!IsEnabled())
		return;

	m_pUIName->SetText((pItem) ? pItem->getName() : CInventoryItem::readName(strSection.c_str()));

	{
		const float fWeight{ (pItem) ? pItem->Weight() : CurrentGameUI()->GetActorMenu().CalcItemWeight(*strSection) };
		auto strKg{ CStringTable().translate("st_kg") };
		xr_sprintf(buffer, "%3.2f %s", fWeight, strKg.c_str());
		m_pUIWeight->SetText(buffer);
	}

	{
		const float fVolume{ (pItem) ? pItem->Volume() : CurrentGameUI()->GetActorMenu().CalcItemVolume(*strSection) };
		auto strLi{ CStringTable().translate("st_li") };
		xr_sprintf(buffer, "%3.2f %s", fVolume, strLi.c_str());
		m_pUIVolume->SetText(buffer);
	}

	{
		const float fCondition{ (pItem) ? pItem->GetCondition() : 1.F };
		const auto strCondition{ conditionTiers.getTier(fCondition) };
		xr_sprintf(buffer, "%s: %s", CStringTable().translate("st_condition").c_str(), CStringTable().translate(strCondition).c_str());
		m_pUICondition->SetText(buffer);
	}

	m_pUIAmount->Show((pItem) ? !!pItem->O.mcast<MAmountable>() : pSettings->r_bool_ex(pCellItem->m_section, "amountable", false));
	if (m_pUIAmount->IsShown())
		set_amount_info(pCellItem, pItem);

	m_pUICost->Show(nItemPrice != u32_max);
	if (m_pUICost->IsShown())
	{
		m_pUICost->SetText(CurrentGameUI()->GetActorMenu().FormatMoney(nItemPrice));
		m_pUICost->Show(true);
	}

	m_pUITradeTip->Show(!!strTradeTip);
	if (m_pUITradeTip->IsShown())
	{
		m_pUITradeTip->SetText(CStringTable().translate(strTradeTip).c_str());
		m_pUITradeTip->AdjustHeightToText();
	}
	
	set_description_info(pCellItem, pItem, !!strTradeTip);

	if (m_pUIItemImage && pItem)
	{
		// Загружаем картинку
		m_pUIItemImage->SetShader(InventoryUtilities::GetEquipmentIconsShader(pCellItem->m_section));
		m_pUIItemImage->GetUIStaticItem().SetTextureRect(pItem->GetIconRect());
		m_pUIItemImage->TextureOn();
		m_pUIItemImage->SetStretchTexture(true);

		Fvector2 v_r;
		pItem->GetIconRect().getsize(v_r);
		v_r.mul(pCellItem->getScale());

		m_pUIItemImage->GetUIStaticItem().SetSize(v_r);
		m_pUIItemImage->SetWidth(v_r.x);
		m_pUIItemImage->SetHeight(v_r.y);
	}
}

void CUIItemInfo::Draw()
{
	if (m_bIsEnabled)
		_super::Draw();
}
