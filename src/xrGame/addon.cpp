#include "stdafx.h"
#include "addon.h"

CAddon::CAddon()
{
	m_SlotType							= 0;
}

void CAddon::Load(LPCSTR section)
{
	inherited::Load						(section);
	m_SlotType							= pSettings->r_string(cNameSect(), "slot_type");
}

void CAddon::Render(Fmatrix* pos)
{
	::Render->set_Transform				(pos);
	::Render->add_Visual				(Visual());
}
