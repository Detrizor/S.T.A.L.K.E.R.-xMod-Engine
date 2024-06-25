#pragma once
#include "muzzle.h"

class CGameObject;

class CSilencer : public CMuzzleBase
{
public:
										CSilencer								(CGameObject* obj, shared_str CR$ section);

private:
	const float							m_sound_suppressing;

public:
	float								getSoundSuppressing					C$	();
};
