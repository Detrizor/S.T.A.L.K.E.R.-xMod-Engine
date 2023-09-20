#include "stdafx.h"
#include "ActorBackpack.h"
#include "Actor.h"
#include "Inventory.h"

CBackpack::CBackpack()
{
	m_flags.set		(FUsingCondition, FALSE);
	m_capacity		= 0.f;
}

CBackpack::~CBackpack()
{
}

void CBackpack::Load(LPCSTR section)
{
	inherited::Load		(section);
	m_flags.set			(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
	m_capacity			= READ_IF_EXISTS(pSettings, r_float, section, "capacity", 10.f);
}

BOOL CBackpack::net_Spawn(CSE_Abstract* DC)
{
	return inherited::net_Spawn(DC);
}

void CBackpack::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
}

void CBackpack::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
}

void CBackpack::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
}

void CBackpack::OnMoveToSlot(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToSlot(previous_place);
}

void CBackpack::OnMoveToRuck(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToRuck(previous_place);
}

void CBackpack::Hit(float hit_power, ALife::EHitType hit_type)
{
	if (IsUsingCondition() == false) return;
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

bool CBackpack::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result		= inherited::install_upgrade_impl(section, test);
	result			|= process_if_exists(section, "capacity", m_capacity, test);
	return			result;
}