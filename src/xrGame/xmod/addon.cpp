#include "stdafx.h"

#include "addon.h"
#include "addon_owner.h"

void CAddon::Load(LPCSTR section)
{
	m_section = section;
	m_SlotType = pSettings->r_string(section, "slot_type");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CAddonObject::Load(LPCSTR section)
{
	inherited::Load(section);
	CAddon::Load(section);
	CAddonOwner::Load(section);
}

DLL_Pure* CAddonObject::_construct()
{
	inherited::_construct			();
	CAddonOwner::_construct			();
	return							this;
}

void CAddonObject::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CAddonOwner::OnEvent(P, type);
}

void CAddonObject::renderable_Render()
{
    inherited::renderable_Render();
	CAddonOwner::renderable_Render();
}

void CAddonObject::UpdateAddonsTransform()
{
	CAddonOwner::UpdateAddonsTransform();
}

void CAddonObject::render_hud_mode()
{
	inherited::render_hud_mode();
	CAddonOwner::render_hud_mode();
}

float CAddonObject::GetControlInertionFactor() const
{
	float res = inherited::GetControlInertionFactor();
	res *= CAddonOwner::GetControlInertionFactor();
	return res;
}
