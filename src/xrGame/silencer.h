#pragma once
#include "muzzle.h"

class CGameObject;

class CSilencer : public CMuzzle
{
public:
										CSilencer								(CGameObject* obj, shared_str CR$ section);

private:
	const float							m_sound_suppressing;
	const float							m_bullet_speed_k;
	const float							m_fire_dispersion_base_k;

public:
	float								getBulletSpeedK						C$	()		{ return m_bullet_speed_k; }
	float								getFireDispersionBaseK				C$	()		{ return m_fire_dispersion_base_k; }

	float								getSoundSuppressing					C$	();
};
