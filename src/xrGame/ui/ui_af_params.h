#pragma once

#include "UIWindow.h"
#include "Artefact.h"

class CUIXml;
class CArtefact;
class CUIMiscInfoItem;

class CUIArtefactParams : public CUIWindow
{
public:
	CUIArtefactParams();
	~CUIArtefactParams() override;

	void initFromXml(CUIXml& xmlDoc);

private:
	void set_info_item(CUIMiscInfoItem* item, float value, float& h);

public:
	void setInfo(CArtefact* pArtefact, LPCSTR strSection);

private:
	xptr<CUIMiscInfoItem> m_pRadiation{};
	xptr<CUIMiscInfoItem> m_pAbsorbations[CArtefact::eAbsorbationTypeMax]{};
	xptr<CUIMiscInfoItem> m_pRestores[CArtefact::eRestoreTypeMax]{};
	xptr<CUIMiscInfoItem> m_pWeightDump{};
	xptr<CUIMiscInfoItem> m_pArmor{};
};
