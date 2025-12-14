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

public:
	void setInfo(CArtefact* pArtefact, LPCSTR strSection);

private:
	xarr<xptr<CUIMiscInfoItem>, CArtefact::eAbsorbationTypeMax> m_pAbsorbations{ this };
	xarr<xptr<CUIMiscInfoItem>, CArtefact::eRestoreTypeMax> m_pRestores{ this };

	xptr<CUIMiscInfoItem> m_pRadiation{ this };
	xptr<CUIMiscInfoItem> m_pWeightDump{ this };
	xptr<CUIMiscInfoItem> m_pArmor{ this };
};
