#include "stdafx.h"
#include "CUIMiscInfo.h"

#include "UIStatic.h"
#include "UIXmlInit.h"
#include "UICellItem.h"
#include "CUIMiscInfoItem.h"

#include "Artefact.h"
#include "addon_owner.h"
#include "string_table.h"
#include "item_container.h"
#include "artefact_module.h"

CUIMiscInfo::CUIMiscInfo() = default;
CUIMiscInfo::~CUIMiscInfo() = default;

void CUIMiscInfo::initFromXml(CUIXml& xmlDoc)
{
	auto strBaseNode{ "misc_info" };

	CUIXmlInit::InitWindow(xmlDoc, strBaseNode, 0, this);
	auto pStoredRoot{ xmlDoc.GetLocalRoot() };
	xmlDoc.SetLocalRoot(xmlDoc.NavigateToNode(strBaseNode, 0));

	m_pMagazineAmmoType->init(xmlDoc, "magazine_ammo_type");
	m_pMagazineAmmoType->SetCaption(CStringTable().translate("ui_ammo_type").c_str());

	m_pMagazineCapacity->init(xmlDoc, "magazine_capacity");
	m_pMagazineCapacity->SetCaption(CStringTable().translate("ui_capacity").c_str());

	m_pContainerCapacity->init(xmlDoc, "container_capacity");
	m_pContainerCapacity->SetCaption(CStringTable().translate("ui_capacity").c_str());

	m_pContainerArtefactIsolation->init(xmlDoc, "container_artefact_isolation");
	m_pContainerArtefactIsolation->SetCaption(CStringTable().translate("ui_artefact_isolation").c_str());
	m_pContainerArtefactIsolation->SetStrValue("");

	m_pArtModuleMaxRadiationLimit->init(xmlDoc, "art_module_max_radiation_limit");
	m_pArtModuleMaxRadiationLimit->SetCaption(CStringTable().translate("ui_max_artefact_radiation_limit").c_str());

	m_pArtModuleRadiation->init(xmlDoc, "art_module_radiation");
	m_pArtModuleRadiation->SetCaption(CStringTable().translate("st_radiation").c_str());

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIMiscInfo::set_magaizine_info(CUICellItem* pCellItem, float& h)
{
	if (!ItemCategory(pCellItem->m_section, "magazine"))
		return;

	auto strAmmoSlotType{ pSettings->r_string(pCellItem->m_section, "ammo_slot_type") };
	auto strSlotName{ CAddonSlot::getSlotName(strAmmoSlotType) };
	m_pMagazineAmmoType->SetStrValue(CStringTable().translate(strSlotName).c_str());
	h += m_pMagazineAmmoType->GetWndSize().y;
	AttachChild(m_pMagazineAmmoType.get());

	float fCapacity{ pSettings->r_float(pCellItem->m_section, "capacity") };
	m_pMagazineCapacity->SetValue(fCapacity);
	m_pMagazineCapacity->SetY(h);
	h += m_pMagazineCapacity->GetWndSize().y;
	AttachChild(m_pMagazineCapacity.get());
}

void CUIMiscInfo::set_container_info(CUICellItem* pCellItem, float& h)
{
	auto pItem{ pCellItem->getItem() };
	auto pContainer{ (pItem) ? pItem->O.getModule<MContainer>() : nullptr };
	if (!pContainer && !(pSettings->r_bool_ex(pCellItem->m_section, "container", false) && !pSettings->r_string(pCellItem->m_section, "supplies")))
		return;

	float fCapacity = (pContainer) ? pContainer->GetCapacity() : pSettings->r_float(pCellItem->m_section, "capacity");
	m_pContainerCapacity->SetValue(fCapacity);
	m_pContainerCapacity->SetY(h);
	h += m_pContainerCapacity->GetWndSize().y;
	AttachChild(m_pContainerCapacity.get());

	if ((pContainer) ? pContainer->ArtefactIsolation(true) : pSettings->r_bool(pCellItem->m_section, "artefact_isolation"))
	{
		m_pContainerArtefactIsolation->SetY(h);
		h += m_pContainerArtefactIsolation->GetWndSize().y;
		AttachChild(m_pContainerArtefactIsolation.get());
	}

	if (pContainer && !pContainer->ArtefactIsolation() && !pContainer->Empty())
	{
		auto artefactIt{ pContainer->Items().find_if([](auto&& item) { return item->O.scast<CArtefact*>(); }) };
		if (artefactIt != pContainer->Items().end())
		{
			auto pArtefact = (*artefactIt)->O.scast<CArtefact*>();
			float fRadiation{ pArtefact->getRadiation() };
			if (fMore(fRadiation, 0.F))
			{
				m_pArtModuleRadiation->SetValue(fRadiation);
				m_pArtModuleRadiation->SetY(h);
				h += m_pArtModuleRadiation->GetWndSize().y;
				AttachChild(m_pArtModuleRadiation.get());
			}
		}
	}
}

void CUIMiscInfo::set_artefact_module_info(CUICellItem* pCellItem, float& h)
{
	auto pItem{ pCellItem->getItem() };
	float fMaxArtefactRadiationLimit{ 0.F };
	if (pItem)
	{
		if (auto pArtModule{ pItem->O.getModule<MArtefactModule>() })
			fMaxArtefactRadiationLimit = pArtModule->getMaxArtefactRadiationLimit();
	}
	else
		pSettings->w_float_ex(fMaxArtefactRadiationLimit, pCellItem->m_section, "max_artefact_radiation_limit");

	if (fMaxArtefactRadiationLimit > 0.F)
	{
		m_pArtModuleMaxRadiationLimit->SetValue(fMaxArtefactRadiationLimit);
		m_pArtModuleMaxRadiationLimit->SetY(h);
		h += m_pArtModuleMaxRadiationLimit->GetWndSize().y;
		AttachChild(m_pArtModuleMaxRadiationLimit.get());
	}
}

void CUIMiscInfo::setInfo(CUICellItem* pCellItem)
{
	DetachAll();
	float h{ 0.F };

	set_magaizine_info(pCellItem, h);
	set_container_info(pCellItem, h);
	set_artefact_module_info(pCellItem, h);

	SetHeight(h);
}
