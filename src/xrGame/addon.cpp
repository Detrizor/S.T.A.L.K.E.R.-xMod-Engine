#include "stdafx.h"
#include "addon.h"
#include "GameObject.h"

CAddon::CAddon(CGameObject* obj) : CModule(obj)
{
	m_SlotType							= pSettings->r_string(O.cNameSect(), "slot_type");
}

void CAddon::Render(Fmatrix* pos)
{
	::Render->set_Transform				(pos);
	::Render->add_Visual				(O.Visual());
}
