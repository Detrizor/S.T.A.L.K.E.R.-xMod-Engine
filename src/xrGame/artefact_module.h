#pragma once
#include "module.h"

class MAmountable;

class MArtefactModule : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mArtefactModule; }

public:
										MArtefactModule							(CGameObject* obj);

private:
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);

private:
	MAmountable PC$						m_amountable_ptr;
	const float							m_max_artefact_radiation_limit;
	const int							m_modes_count;

	int									m_cur_mode								= 0;

public:
	float								getMaxArtefactRadiationLimit		C$	()		{ return m_max_artefact_radiation_limit; }
	int									getMode								C$	()		{ return m_cur_mode; }
	float								getArtefactRadiationLimit			C$	();

	void								setMode									(int val);
};
