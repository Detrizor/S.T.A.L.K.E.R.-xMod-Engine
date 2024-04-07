#pragma once

#include "module.h"

class CGrenadeLauncher : public CModule
{
public:
										CGrenadeLauncher						(CGameObject* obj, shared_str CR$ section);

private:
	float								m_fGrenadeVel;
	shared_str							m_sFlameParticles;
	Fvector								m_sight_offset[2];

public:
	float								GetGrenadeVel						C$	()		{ return m_fGrenadeVel; }
	shared_str CR$						FlameParticles						C$	()		{ return m_sFlameParticles; }
	Fvector CP$							getSightOffset						C$	()		{ return m_sight_offset; }
};
