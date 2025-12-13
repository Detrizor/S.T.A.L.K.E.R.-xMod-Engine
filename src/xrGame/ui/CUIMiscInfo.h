#pragma once

#include "UIWindow.h"

class CUIXml;
class CUIStatic;
class MContainer;
class CUICellItem;
class CUIMiscInfoItem;

class CUIMiscInfo : public CUIWindow
{
public:
	CUIMiscInfo();
	~CUIMiscInfo() override;

	void initFromXml(CUIXml& xmlDoc);

private:
	void set_magaizine_info(CUICellItem* pCellItem, float& h);
	void set_container_info(CUICellItem* pCellItem, float& h);
	void set_artefact_module_info(CUICellItem* pCellItem, float& h);

public:
	void setInfo(CUICellItem* pCellItem);

private:
	xptr<CUIMiscInfoItem> m_pMagazineAmmoType{};
	xptr<CUIMiscInfoItem> m_pMagazineCapacity{};

	xptr<CUIMiscInfoItem> m_pContainerCapacity{};
	xptr<CUIMiscInfoItem> m_pContainerArtefactIsolation{};

	xptr<CUIMiscInfoItem> m_pArtModuleArtefactActivateCharge{};
};
