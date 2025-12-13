#pragma once
#include "module.h"

class CArtefact;
class MAmountable;

class MArtefactModule : public CModule
{
public:
	static EModuleTypes mid() { return mArtefactModule; }

public:
	explicit MArtefactModule(CGameObject* obj);

private:
	void sSyncData(CSE_ALifeDynamicObject* se_obj, bool save) override;

public:
	float	getArtefactActivateCharge	() const	{ return m_fArtefactActivateCharge; }
	int		getMode						() const	{ return m_nCurMode; }

	float getAllowedArtefactCharge(const CArtefact* artefact) const;

	void setMode(int val);

private:
	MAmountable* const	m_pAmountable;
	const float			m_fArtefactActivateCharge;
	const int			m_nModesCount;

	int m_nCurMode{ 0 };
};
