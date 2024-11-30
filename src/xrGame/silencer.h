#pragma once
#include "muzzle.h"

class CGameObject;

class CSilencer : public MMuzzle
{
public:
	static EModuleTypes					mid										()		{ return mSilencer; }

public:
										CSilencer								(CGameObject* obj, shared_str CR$ section);

private:
	const float							m_sound_suppressing;
	const float							m_bullet_speed_k;

public:
	float								getBulletSpeedK						C$	()		{ return m_bullet_speed_k; }
	float								getSoundSuppressing					C$	();
};
