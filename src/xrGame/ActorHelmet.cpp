////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ActorHelmet.h"
#include "Actor.h"
#include "Inventory.h"
#include "Torch.h"
#include "BoneProtections.h"
#include "../Include/xrRender/Kinematics.h"

CHelmet::CHelmet()
{
	m_flags.set(FUsingCondition, TRUE);
	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

	m_boneProtection	= xr_new<SBoneProtections>();
}

CHelmet::~CHelmet()
{
	xr_delete(m_boneProtection);
}

void CHelmet::Load(LPCSTR section) 
{
	inherited::Load					(section);

	m_HitTypeProtection[ALife::eHitTypeBurn]			= pSettings->r_float(section,"burn_protection");
	m_HitTypeProtection[ALife::eHitTypeShock]			= pSettings->r_float(section,"shock_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation]		= pSettings->r_float(section,"radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]	= pSettings->r_float(section,"chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic]		= pSettings->r_float(section,"telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeLightBurn]		= m_HitTypeProtection[ALife::eHitTypeBurn];

	m_NightVisionSect				= READ_IF_EXISTS(pSettings, r_string, section, "nightvision_sect", "");
	m_fRecuperationFactor			= READ_IF_EXISTS(pSettings, r_float, section, "recuperation_factor", 0.f);

	m_BonesProtectionSect			= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "" );
	m_fArmorLevel					= READ_IF_EXISTS(pSettings, r_float, section, "armor_level",  0.f);
	m_fShowNearestEnemiesDistance	= READ_IF_EXISTS(pSettings, r_float, section, "nearest_enemies_show_dist",  0.0f );

	// Added by Axel, to enable optional condition use on any item
	m_flags.set						(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
}

void CHelmet::ReloadBonesProtection()
{
	CObject* parent = smart_cast<CObject*>(Level().CurrentViewEntity()); //TODO: FIX THIS OR NPC Can't wear outfit without resetting actor 

	if(parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload( m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}

BOOL CHelmet::net_Spawn(CSE_Abstract* DC)
{
	ReloadBonesProtection	();
	BOOL res				= inherited::net_Spawn(DC);
	return					(res);
}

void CHelmet::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	P.w_float_q8(GetCondition(),0.0f,1.0f);
}

void CHelmet::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
	float _cond;
	P.r_float_q8(_cond,0.0f,1.0f);
	SetCondition(_cond);
}

void CHelmet::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
//	ReloadBonesProtection();
}

void CHelmet::OnMoveToSlot(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToSlot		(previous_place);
	if (m_pInventory && (previous_place.type==eItemPlaceSlot))
	{
		CActor* pActor = smart_cast<CActor*> (H_Parent());
		if (pActor)
		{
			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if(pTorch && pTorch->GetNightVisionStatus())
				pTorch->SwitchNightVision(true, false);
		}
	}
}

void CHelmet::OnMoveToRuck(const SInvItemPlace& previous_place)
{
	inherited::OnMoveToRuck		(previous_place);
	if (m_pInventory && (previous_place.type==eItemPlaceSlot))
	{
		CActor* pActor = smart_cast<CActor*> (H_Parent());
		if (pActor)
		{
			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if(pTorch)
				pTorch->SwitchNightVision(false);
		}
	}
}

void CHelmet::Hit(float hit_power, ALife::EHitType hit_type)
{
	if (IsUsingCondition() == false) return;
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

float CHelmet::GetHitTypeProtection(ALife::EHitType hit_type)
{
	float protection;
	if (hit_type == ALife::eHitTypeFireWound || hit_type == ALife::eHitTypeStrike || hit_type == ALife::eHitTypeWound || hit_type == ALife::eHitTypeWound_2)
		protection = GetBoneArmor(15);
	else
		protection = m_HitTypeProtection[hit_type];
	protection *= GetConditionToWork();
	return protection;
}

float CHelmet::GetBoneArmor(s16 element)
{
	return m_boneProtection->getBoneArmor(element);
}

float CHelmet::GetBoneArmorLevel(s16 element)
{
	return m_boneProtection->getBoneArmorLevel(element);
}

bool CHelmet::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl(section, test);

	result |= process_if_exists(section, "burn_protection", m_HitTypeProtection[ALife::eHitTypeBurn], test);
	result |= process_if_exists(section, "shock_protection", m_HitTypeProtection[ALife::eHitTypeShock], test);
	result |= process_if_exists(section, "radiation_protection", m_HitTypeProtection[ALife::eHitTypeRadiation], test);
	result |= process_if_exists(section, "telepatic_protection", m_HitTypeProtection[ALife::eHitTypeTelepatic], test);
	result |= process_if_exists(section, "chemical_burn_protection", m_HitTypeProtection[ALife::eHitTypeChemicalBurn], test);

	LPCSTR str;
	bool result2 = process_if_exists(section, "nightvision_sect", str, test);
	if (result2 && !test)
		m_NightVisionSect._set(str);
	result |= result2;

	result |= process_if_exists(section, "nearest_enemies_show_dist", m_fShowNearestEnemiesDistance, test);

	result |= process_if_exists(section, "armor_level", m_fArmorLevel, test);
	result2 = process_if_exists(section, "bones_koeff_protection", str, test);
	if (result2 && !test)
	{
		m_BonesProtectionSect = str;
		ReloadBonesProtection();
	}
	result |= result2;

	result2						= process_if_exists(section, "bones_koeff_protection_add_sect", str, test);
	if (result2 && !test)
	{
		float value				= 0.f;
		process_if_exists		(section, "bones_koeff_protection_add_value", value, test);
		AddBonesProtection		(str, value);
	}
	result						|= result2;

	return result;
}

void CHelmet::AddBonesProtection(LPCSTR bones_section, float value)
{
	CObject* parent					= smart_cast<CObject*>(Level().CurrentViewEntity()); //TODO: FIX THIS OR NPC Can't wear outfit without resetting actor 
	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->add		(bones_section, smart_cast<IKinematics*>(parent->Visual()), value);
}