#include "stdafx.h"
#include "silencer.h"
#include "addon.h"

CSilencer::CSilencer(CGameObject* obj, shared_str CR$ section) : CMuzzle(obj, section),
m_sound_suppressing(pSettings->r_float(section, "sound_suppressing")),
m_bullet_speed_k(pSettings->r_float(section, "bullet_speed_k")),
m_fire_dispersion_base_k(pSettings->r_float(section, "fire_dispersion_base_k"))
{
}

float CSilencer::getSoundSuppressing() const
{
	return								m_sound_suppressing * I->GetCondition();
}
