#pragma once
#include "module.h"

class CArtefact;
class MAmountable;
class CUIArtefactModuleCellItem;

class MArtefactModule : public CModule
{
public:
	static EModuleTypes mid() { return mArtefactModule; }

public:
	explicit MArtefactModule(CGameObject* obj);

private:
	void sSyncData(CSE_ALifeDynamicObject* se_obj, bool save) override;
	xptr<CUICellItem> sCreateIcon() override;

public:
	float getArtefactActivateCharge	() const { return m_fArtefactActivateCharge; }
	float getMode					() const { return m_fMode; }

	float getAllowedArtefactCharge(const CArtefact* artefact) const;

	void setMode(float fValue);

private:
	const float m_fArtefactActivateCharge;

	float m_fMode{ 0.F };
	CUIArtefactModuleCellItem* m_pIcon{ nullptr };
};
