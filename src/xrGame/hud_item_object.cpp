#include "stdafx.h"
#include "hud_item_object.h"

CHudItemObject::CHudItemObject()
{
}

CHudItemObject::~CHudItemObject()
{
}

DLL_Pure *CHudItemObject::_construct()
{
	CInventoryItemObjectOld::_construct();
	CHudItem::_construct();
	return						(this);
}

void CHudItemObject::Load(LPCSTR section)
{
	CInventoryItemObjectOld::Load(section);
	CHudItem::Load(section);
}

bool CHudItemObject::Action(u16 cmd, u32 flags)
{
	if (CInventoryItemObjectOld::Action(cmd, flags))
		return					(true);
	return						(CHudItem::Action(cmd, flags));
}

void CHudItemObject::SwitchState(u32 S)
{
	CHudItem::SwitchState(S);
}

void CHudItemObject::OnStateSwitch(u32 S, u32 oldState)
{
	CHudItem::OnStateSwitch(S, oldState);
}

void CHudItemObject::OnMoveToRuck(const SInvItemPlace& prev)
{
	CInventoryItemObjectOld::OnMoveToRuck(prev);
	CHudItem::OnMoveToRuck(prev);
}

void CHudItemObject::OnEvent(NET_Packet& P, u16 type)
{
	CInventoryItemObjectOld::OnEvent(P, type);
	CHudItem::OnEvent(P, type);
}

void CHudItemObject::OnH_A_Chield()
{
	CHudItem::OnH_A_Chield();
	CInventoryItemObjectOld::OnH_A_Chield();
}

void CHudItemObject::OnH_B_Chield()
{
	CInventoryItemObjectOld::OnH_B_Chield();
	CHudItem::OnH_B_Chield();
}

void CHudItemObject::OnH_B_Independent(bool just_before_destroy)
{
	CHudItem::OnH_B_Independent(just_before_destroy);
	CInventoryItemObjectOld::OnH_B_Independent(just_before_destroy);
}

void CHudItemObject::OnH_A_Independent()
{
	CHudItem::OnH_A_Independent();
	CInventoryItemObjectOld::OnH_A_Independent();
}

BOOL CHudItemObject::net_Spawn(CSE_Abstract* DC)
{
	return						(
		CInventoryItemObjectOld::net_Spawn(DC) &&
		CHudItem::net_Spawn(DC)
		);
}

void CHudItemObject::net_Destroy()
{
	CHudItem::net_Destroy();
	CInventoryItemObjectOld::net_Destroy();
}

bool CHudItemObject::ActivateItem()
{
	return			(CHudItem::ActivateItem());
}

void CHudItemObject::DeactivateItem()
{
	CHudItem::DeactivateItem();
}

void CHudItemObject::UpdateCL()
{
	CInventoryItemObjectOld::UpdateCL();
	CHudItem::UpdateCL();
}

void CHudItemObject::renderable_Render()
{
	CHudItem::renderable_Render();
}

void CHudItemObject::on_renderable_Render()
{
	CInventoryItemObjectOld::renderable_Render();
}
