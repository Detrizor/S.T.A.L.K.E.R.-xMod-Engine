#pragma once

#include "UIWindow.h"

class CUIXml;
class CArtefact;
class CUIMiscInfoItem;

enum EAbsorbationTypes
{
	eBurnImmunity,
	eShockImmunity,
	eChemBurnImmunity,
	eRadiationImmunity,
	eTelepaticImmunity,
	eAbsorbationTypeMax
};

enum EConditionRestoreTypes {
	eRadiationSpeed = 0,
	ePainkillSpeed,
	eRegenerationSpeed,
	eRecuperationSpeed,
	eRestoreTypeMax
};

class CUIArtefactParams : public CUIWindow
{
public:
	CUIArtefactParams();
	~CUIArtefactParams() override;

	void initFromXml(CUIXml& xmlDoc);

private:
	void set_info_item(CUIMiscInfoItem* item, float value, float& h);

public:
	void setInfo(LPCSTR section, CArtefact* art);

private:
	xptr<CUIMiscInfoItem> m_radiation{};
	xptr<CUIMiscInfoItem> m_absorbation_item[eAbsorbationTypeMax]{};
	xptr<CUIMiscInfoItem> m_restore_item[eRestoreTypeMax]{};
	xptr<CUIMiscInfoItem> m_weight_dump{};
	xptr<CUIMiscInfoItem> m_armor{};
};
