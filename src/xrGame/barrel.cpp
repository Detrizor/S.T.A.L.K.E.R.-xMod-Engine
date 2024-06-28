#include "stdafx.h"
#include "barrel.h"
#include "addon.h"

CBarrel::CBarrel(CGameObject* obj, shared_str CR$ section) : CMuzzleBase(obj, section),
m_length(pSettings->r_float(section, "length"))
{
}
