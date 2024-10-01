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
#include "ui/UIOutfitInfo.h"
#include "inventory_item_amountable.h"

#define MAIN_BONE 15
const float BASIC_HEALTH = pSettings->r_float("damage_manager", "helmet_basic_health");

CHelmet::CHelmet()
{
	m_flags.set					(FUsingCondition, TRUE);
	m_boneProtection			= xr_new<SBoneProtections>();
	m_BonesProtectionSect		= NULL;
	m_fHealth					= 0.f;
	m_HitTypeProtection.resize	(ALife::eHitTypeMax);
	for (int i = 0; i < ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i]	= 0.f;
}

CHelmet::~CHelmet()
{
	xr_delete(m_boneProtection);
}

void CHelmet::Load(LPCSTR section) 
{
	inherited::Load					(section);
	
	extern LPCSTR										protection_sections[];
	for (int i = 0; i < eProtectionTypeMax; i++)
		m_HitTypeProtection[i]							= pSettings->r_float(section, protection_sections[i]);
	m_HitTypeProtection[ALife::eHitTypeLightBurn]		= m_HitTypeProtection[ALife::eHitTypeBurn];

	m_NightVisionSect				= READ_IF_EXISTS(pSettings, r_string, section, "nightvision_sect", "");
	if (m_NightVisionSect.size())
		getModule<MAmountable>()->SetDepletionSpeed(pSettings->r_float("nightvision_depletes", *m_NightVisionSect));

	m_fRecuperationFactor			= READ_IF_EXISTS(pSettings, r_float, section, "recuperation_factor", 0.f);

	m_BonesProtectionSect			= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "" );
	m_fArmorLevel					= READ_IF_EXISTS(pSettings, r_float, section, "armor_level",  0.f);
	m_fShowNearestEnemiesDistance	= READ_IF_EXISTS(pSettings, r_float, section, "nearest_enemies_show_dist",  0.0f );

	// Added by Axel, to enable optional condition use on any item
	m_flags.set						(FUsingCondition, READ_IF_EXISTS(pSettings, r_BOOL, section, "use_condition", TRUE));

	m_fHealth						= pSettings->r_float(section, "health");
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
	return					inherited::net_Spawn(DC);
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

	hit_power			*= GetHitImmunity(hit_type);
	hit_power			*= BASIC_HEALTH / Health();
	ChangeCondition		(-hit_power);
}

float CHelmet::GetHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type] * sqrt(GetConditionToWork());
}

float CHelmet::GetBoneArmor(s16 element)
{
	float res = m_boneProtection->getBoneArmor(element ? element : MAIN_BONE);
	if (fMore(res, 0.f))
		res *= sqrt(GetConditionToWork());
	return res;
}

float CHelmet::GetBoneArmorLevel(s16 element)
{
	return m_boneProtection->getBoneArmorLevel(element);
}

bool CHelmet::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl(section, test);
	
	extern LPCSTR protection_sections[];
	for (int i = 0; i < eProtectionTypeMax; i++)
		result |= process_if_exists(section, protection_sections[i], m_HitTypeProtection[i], test);
	m_HitTypeProtection[ALife::eHitTypeLightBurn] = m_HitTypeProtection[ALife::eHitTypeBurn];

	LPCSTR str;
	bool result2 = process_if_exists(section, "nightvision_sect", str, test);
	if (result2 && !test)
	{
		m_NightVisionSect._set			(str);
		getModule<MAmountable>()->SetDepletionSpeed(pSettings->r_float("nightvision_depletes", str));
	}
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

float CHelmet::Aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eGetBar:
			return						(fLess(GetCondition(), 1.f)) ? GetCondition() : -1.f;
	}

	return								inherited::Aboba(type, data, param);
}
