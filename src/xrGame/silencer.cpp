#include "stdafx.h"
#include "silencer.h"
#include "addon.h"

CSilencer::CSilencer(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_section							= section;
	if (obj->Cast<CAddon*>())
		m_muzzle_point_shift			= pSettings->r_float(section, "muzzle_point_shift");
}
