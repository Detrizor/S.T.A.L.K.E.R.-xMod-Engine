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

	m_pMagazineAmmoType->init(xmlDoc, "magazine_ammo_type", false);
	m_pMagazineCapacity->init(xmlDoc, "magazine_capacity", false);
	m_pContainerCapacity->init(xmlDoc, "container_capacity", false);
	m_pContainerArtefactIsolation->init(xmlDoc, "container_artefact_isolation", false);
	m_pArtModuleArtefactActivateCharge->init(xmlDoc, "art_module_artefact_activate_charge", false);

	xmlDoc.SetLocalRoot(pStoredRoot);
}

void CUIMiscInfo::set_magaizine_info(CUICellItem* pCellItem, float& h)
{
	if (!ItemCategory(pCellItem->m_section, "magazine"))
		return;

	auto strAmmoSlotType{ pSettings->r_string(pCellItem->m_section, "ammo_slot_type") };
	auto strSlotName{ CAddonSlot::getSlotName(strAmmoSlotType) };
	m_pMagazineAmmoType->setStrValue(CStringTable().translate(strSlotName).c_str(), h);

	float fCapacity{ pSettings->r_float(pCellItem->m_section, "capacity") };
	m_pMagazineCapacity->setValue(fCapacity, h);
}

void CUIMiscInfo::set_container_info(CUICellItem* pCellItem, float& h)
{
	auto pItem{ pCellItem->getItem() };
	auto pContainer{ (pItem) ? pItem->O.getModule<MContainer>() : nullptr };
	if (!pContainer && !pSettings->r_bool_ex(pCellItem->m_section, "container", false))
		return;

	if (pSettings->r_string(pCellItem->m_section, "supplies"))
		return;

	if (pItem && pItem->O.getModule<MArtefactModule>() || !pItem && pSettings->r_bool_ex(pCellItem->m_section, "artefact_module", false))
		return;

	float fCapacity = (pContainer) ? pContainer->GetCapacity() : pSettings->r_float(pCellItem->m_section, "capacity");
	m_pContainerCapacity->setValue(fCapacity, h);

	if ((pContainer) ? pContainer->ArtefactIsolation(true) : pSettings->r_bool(pCellItem->m_section, "artefact_isolation"))
		m_pContainerArtefactIsolation->setStrValue("", h);
}

void CUIMiscInfo::set_artefact_module_info(CUICellItem* pCellItem, float& h)
{
	auto pItem{ pCellItem->getItem() };
	float fArtefactActivateCharge{ 0.F };
	if (pItem)
	{
		if (auto pArtModule{ pItem->O.getModule<MArtefactModule>() })
			fArtefactActivateCharge = pArtModule->getArtefactActivateCharge();
	}
	else
		pSettings->w_float_ex(fArtefactActivateCharge, pCellItem->m_section, "artefact_activate_charge");

	if (fMore(fArtefactActivateCharge, 0.F))
		m_pArtModuleArtefactActivateCharge->setValue(fArtefactActivateCharge, h);
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
