#include "stdafx.h"
#include "silencer.h"
#include "addon.h"

CSilencer::CSilencer(CGameObject* obj, shared_str CR$ section) : CMuzzleBase(obj, section),
m_sound_suppressing(pSettings->r_float(section, "sound_suppressing"))
{
}

float CSilencer::getSoundSuppressing() const
{
	return								m_sound_suppressing * I->GetCondition();
}
