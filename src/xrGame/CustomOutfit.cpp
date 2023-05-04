////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "customoutfit.h"
#include "../xrphysics/PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "../Include/xrRender/Kinematics.h"
#include "player_hud.h"
#include "ui/UIOutfitInfo.h"
#include "inventory_item_amountable.h"

#define MAIN_BONE 11
const float BASIC_HEALTH = pSettings->r_float("damage_manager", "armor_basic_health");

CCustomOutfit::CCustomOutfit()
{
	m_flags.set					(FUsingCondition, TRUE);
	m_boneProtection			= xr_new<SBoneProtections>();
	m_artefact_count			= 0;
	m_BonesProtectionSect		= NULL;
	m_fHealth					= 0.f;
	m_HitTypeProtection.resize	(ALife::eHitTypeMax);
	for (int i = 0; i < ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i]	= 0.f;
}

CCustomOutfit::~CCustomOutfit() 
{
	xr_delete(m_boneProtection);
}

BOOL CCustomOutfit::net_Spawn(CSE_Abstract* DC)
{
	ReloadBonesProtection	();
	BOOL res				= inherited::net_Spawn(DC);
	return					(res);
}

void CCustomOutfit::net_Export(NET_Packet& P)
{
	inherited::net_Export	(P);
	P.w_float_q8			(GetCondition(),0.0f,1.0f);
}

void CCustomOutfit::net_Import(NET_Packet& P)
{
	inherited::net_Import	(P);
	float _cond;
	P.r_float_q8			(_cond,0.0f,1.0f);
	SetCondition			(_cond);
}

void CCustomOutfit::Load(LPCSTR section) 
{
	inherited::Load				(section);
	
	extern LPCSTR										protection_sections[];
	for (int i = 0; i < eProtectionTypeMax; i++)
		m_HitTypeProtection[i]							= pSettings->r_float(section, protection_sections[i]);
	m_HitTypeProtection[ALife::eHitTypeLightBurn]		= m_HitTypeProtection[ALife::eHitTypeBurn];

	m_NightVisionSect			= READ_IF_EXISTS(pSettings, r_string, section, "nightvision_sect", "");
	if (m_NightVisionSect.size())
		Cast<CAmountable*>()->SetDepletionSpeed(pSettings->r_float("nightvision_depletes", *m_NightVisionSect));

	m_ActorVisual				= READ_IF_EXISTS(pSettings, r_string, section, "actor_visual", NULL);
	m_ef_equipment_type			= pSettings->r_u32(section,"ef_equipment_type");
	
	m_artefact_count 			= READ_IF_EXISTS(pSettings, r_u32, section, "artefact_count", 0);
	m_fWeightDump				= READ_IF_EXISTS(pSettings, r_float, section, "weight_dump", 0.f);
	m_fRecuperationFactor		= READ_IF_EXISTS(pSettings, r_float, section, "recuperation_factor", 0.f);
	m_fDrainFactor				= READ_IF_EXISTS(pSettings, r_float, section, "drain_factor", 0.f);
	m_fPowerLoss				= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 0.f);
	clamp						(m_fPowerLoss, 0.f, 1.0f);
	
	m_capacity					= 0.f;
	LPCSTR pockets				= READ_IF_EXISTS(pSettings, r_string, section, "pockets", 0);
	if (pockets)
		GetPockets				(pockets);

	m_BonesProtectionSect		= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "");
	m_fArmorLevel				= READ_IF_EXISTS(pSettings, r_float, section, "armor_level",  0.f);
	bIsHelmetAvaliable			= !!READ_IF_EXISTS(pSettings, r_bool, section, "helmet_available", true);

	// Added by Axel, to enable optional condition use on any item
	m_flags.set					(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));

	m_fHealth					= pSettings->r_float(section, "health");
}

void CCustomOutfit::GetPockets(LPCSTR pockets)
{
	string128					buf;
	for (u8 i = 0, cnt = (u8)_GetItemCount(pockets); i < cnt; i++)
	{
		float capacity			= (float)atof(_GetItem(pockets, i, buf));
		m_pockets.push_back		(capacity);
		m_capacity				+= capacity;
	}
}

void CCustomOutfit::ReloadBonesProtection()
{
	CObject* parent = smart_cast<CObject*>(Level().CurrentViewEntity()); //TODO: FIX THIS OR NPC Can't wear outfit without resetting actor 

	if(parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload( m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}

void CCustomOutfit::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power			*= GetHitImmunity(hit_type);
	hit_power			*= BASIC_HEALTH / Health();
	ChangeCondition		(-hit_power);
}

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type] * sqrt(GetConditionToWork());
}

float CCustomOutfit::GetBoneArmor(s16 element)
{
	float res = m_boneProtection->getBoneArmor(element ? element : MAIN_BONE);
	if (fMore(res, 0.f))
		res *= sqrt(GetConditionToWork());
	return res;
}

float CCustomOutfit::GetBoneArmorLevel(s16 element)
{
	return m_boneProtection->getBoneArmorLevel(element);
}

#include "torch.h"
#include "uigamecustom.h"
#include "ui/UIActorMenu.h"
void CCustomOutfit::OnMoveToSlot(const SInvItemPlace& prev)
{
	if (m_pInventory)
	{
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if (pActor)
		{
			ApplySkinModel(pActor, true, false);
			if (prev.type == eItemPlaceSlot && !bIsHelmetAvaliable)
			{
				CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
				if (pTorch && pTorch->GetNightVisionStatus())
					pTorch->SwitchNightVision(true, false);
			}

			if (prev.type == eItemPlaceSlot && prev.slot_id == BaseSlot() && prev.slot_id != CurrSlot())
			{
				m_pInventory->EmptyPockets									();
				CurrentGameUI()->GetActorMenu().UpdatePocketsPresence		();
			}

			if (prev.slot_id == HandSlot() && CurrSlot() == BaseSlot())
				CurrentGameUI()->GetActorMenu().UpdatePocketsPresence();
		}
	}
}

void CCustomOutfit::ApplySkinModel(CActor* pActor, bool bDress, bool bHUDOnly)
{
	if(bDress)
	{
		if(!bHUDOnly && m_ActorVisual.size())
		{
			shared_str NewVisual = NULL;
			char* TeamSection = Game().getTeamSection(pActor->g_Team());
			if (TeamSection)
			{
				if (pSettings->line_exist(TeamSection, *cNameSect()))
				{
					NewVisual = pSettings->r_string(TeamSection, *cNameSect());
					string256 SkinName;

					xr_strcpy(SkinName, pSettings->r_string("mp_skins_path", "skin_path"));
					xr_strcat(SkinName, *NewVisual);
					xr_strcat(SkinName, ".ogf");
					NewVisual._set(SkinName);
				}
			}
			if (!NewVisual.size())
				NewVisual = m_ActorVisual;

			pActor->ChangeVisual(NewVisual);
		}


		if (pActor == Level().CurrentViewEntity())	
			g_player_hud->load(pSettings->r_string(cNameSect(),"player_hud_section"));
	}else
	{
		if (!bHUDOnly && m_ActorVisual.size())
		{
			shared_str DefVisual	= pActor->GetDefaultVisualOutfit();
			if (DefVisual.size())
			{
				pActor->ChangeVisual(DefVisual);
			};
		}

		if (pActor == Level().CurrentViewEntity())	
			g_player_hud->load_default();
	}

}

void CCustomOutfit::OnMoveToRuck(const SInvItemPlace& prev)
{
	if (m_pInventory && prev.type == eItemPlaceSlot)
	{
		CActor* pActor = smart_cast<CActor*> (H_Parent());
		if (pActor)
		{
			ApplySkinModel(pActor, false, false);
			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if (pTorch && !bIsHelmetAvaliable)
				pTorch->SwitchNightVision(false);
		}
	}
}

u32	CCustomOutfit::ef_equipment_type	() const
{
	return		(m_ef_equipment_type);
}

bool CCustomOutfit::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl(section, test);
	
	extern LPCSTR protection_sections[];
	for (int i = 0; i < eProtectionTypeMax; i++)
		result |= process_if_exists(section, protection_sections[i], m_HitTypeProtection[i], test);
	m_HitTypeProtection[ALife::eHitTypeLightBurn] = m_HitTypeProtection[ALife::eHitTypeBurn];

	LPCSTR							str;
	bool result2					= process_if_exists(section, "nightvision_sect", str, test);
	if (result2 && !test)
	{
		m_NightVisionSect._set		(str);
		Cast<CAmountable*>()->SetDepletionSpeed(pSettings->r_float("nightvision_depletes", str));
	}
	result |= result2;

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

	result |= process_if_exists(section, "artefact_count", m_artefact_count, test);
	result |= process_if_exists(section, "weight_dump", m_fWeightDump, test);
	result |= process_if_exists(section, "recuperation_factor", m_fRecuperationFactor, test);
	result |= process_if_exists(section, "drain_factor", m_fDrainFactor, test);
	result |= process_if_exists(section, "power_loss", m_fPowerLoss, test);
	clamp(m_fPowerLoss, 0.0f, 1.f);

	LPCSTR				pockets;
	result2				= process_if_exists(section, "add_pocket", pockets, test);
	if (!result2)
		result2			|= process_if_exists(section, "add_pockets", pockets, test);
	if (result2)
		GetPockets		(pockets);

	return		result;
}

void CCustomOutfit::AddBonesProtection(LPCSTR bones_section, float value)
{
	CObject* parent					= smart_cast<CObject*>(Level().CurrentViewEntity()); //TODO: FIX THIS OR NPC Can't wear outfit without resetting actor 
	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->add		(bones_section, smart_cast<IKinematics*>(parent->Visual()), value);
}

float CCustomOutfit::Aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eGetBar:
			return						(fLess(GetCondition(), 1.f)) ? GetCondition() : -1.f;
	}

	return								inherited::Aboba(type, data, param);
}
