#include "stdafx.h"
#include "entitycondition.h"
#include "inventoryowner.h"
#include "customoutfit.h"
#include "inventory.h"
#include "wound.h"
#include "level.h"
#include "game_cl_base.h"
#include "entity_alive.h"
#include "../Include/xrRender/KinematicsAnimated.h"
#include "../Include/xrRender/Kinematics.h"
#include "object_broker.h"
#include "ActorBackpack.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "actor.h"
#include "artefact.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"

#define MAX_HEALTH 1.0f
#define MIN_HEALTH -0.01f

#define OUTFIT_MAIN_BONE 11
#define HELMET_MAIN_BONE 15

#define MAX_POWER 1.0f
#define MAX_RADIATION 1.0f
#define MAX_PSY_HEALTH 1.0f

HitImmunity::HitTypeSVec CEntityCondition::HitTypeHeadPart;
HitImmunity::HitTypeSVec CEntityCondition::HitTypeScale;

float CEntityCondition::m_fMeleeOnPierceDamageMultiplier;
float CEntityCondition::m_fMeleeOnPierceArmorDamageFactor;

CPowerDependency CEntityCondition::ArmorDamageResistance;
CPowerDependency CEntityCondition::StrikeDamageThreshold;
CPowerDependency CEntityCondition::StrikeDamageResistance;
CPowerDependency CEntityCondition::ExplDamageResistance;

CPowerDependency CEntityCondition::AnomalyDamageThreshold;
CPowerDependency CEntityCondition::AnomalyDamageResistance;
CPowerDependency CEntityCondition::ProtectionDamageResistance;

CEntityConditionSimple::CEntityConditionSimple()
{
	max_health()		= MAX_HEALTH;
	SetHealth			( MAX_HEALTH );
}

CEntityConditionSimple::~CEntityConditionSimple()
{}

CEntityCondition::CEntityCondition(CEntityAlive *object)
:CEntityConditionSimple()
{
	VERIFY				(object);

	m_object			= object;

	m_use_limping_state = false;
	m_iLastTimeCalled	= 0;
	m_bTimeValid		= false;

	m_fPowerMax			= MAX_POWER;
	m_fRadiationMax		= MAX_RADIATION;
	m_fPsyHealthMax		= MAX_PSY_HEALTH;
	m_fEntityMorale		=  m_fEntityMoraleMax = 1.f;


	m_fPower			= MAX_POWER;
	m_fRadiation		= 0;
	m_fPsyHealth		= MAX_PSY_HEALTH;

	m_fMinWoundSize			= 0.00001f;

	
	m_fHealthHitPart		= 1.0f;
	m_fPowerHitPart			= 0.5f;

	m_fDeltaHealth			= 0;
	m_fDeltaPower			= 0;
	m_fDeltaRadiation		= 0;
	m_fDeltaPsyHealth		= 0;

	m_fHealthLost			= 0.f;
	m_pWho					= NULL;
	m_iWhoID				= 0;

	m_WoundVector.clear		();

	m_fKillHitTreshold		= 0;
	m_fLastChanceHealth		= 0;
	m_fInvulnerableTime		= 0;
	m_fInvulnerableTimeDelta= 0;

	m_fArmorDamageBoneScale	= 1.f;
	m_fPierceDamageBoneScale= 1.f;

	m_bIsBleeding			= false;
	m_bCanBeHarmed			= true;
}

CEntityCondition::~CEntityCondition(void)
{
	ClearWounds				();
}

void CEntityCondition::ClearWounds()
{
	for(WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
		xr_delete(*it);
	m_WoundVector.clear();

	m_bIsBleeding = false;
}

void CEntityCondition::LoadCondition(LPCSTR entity_section)
{
	LPCSTR				section = READ_IF_EXISTS(pSettings,r_string,entity_section,"condition_sect",entity_section);

	m_change_v.load		(section,"");

	m_fMinWoundSize			= pSettings->r_float(section,"min_wound_size");
	m_fHealthHitPart		= pSettings->r_float(section,"health_hit_part");
	m_fPowerHitPart			= pSettings->r_float(section,"power_hit_part");

	m_use_limping_state		= !!(READ_IF_EXISTS(pSettings,r_BOOL,section,"use_limping_state",FALSE));
	m_limping_threshold		= READ_IF_EXISTS(pSettings,r_float,section,"limping_threshold",.5f);

	m_fKillHitTreshold		= READ_IF_EXISTS(pSettings,r_float,section,"killing_hit_treshold",0.0f);
	m_fLastChanceHealth		= READ_IF_EXISTS(pSettings,r_float,section,"last_chance_health",0.0f);
	m_fInvulnerableTimeDelta= READ_IF_EXISTS(pSettings,r_float,section,"invulnerable_time",0.0f)/1000.f;
}

void CEntityCondition::LoadTwoHitsDeathParams(LPCSTR section)
{
	m_fKillHitTreshold		= READ_IF_EXISTS(pSettings,r_float,section,"killing_hit_treshold",0.0f);
	m_fLastChanceHealth		= READ_IF_EXISTS(pSettings,r_float,section,"last_chance_health",0.0f);
	m_fInvulnerableTimeDelta= READ_IF_EXISTS(pSettings,r_float,section,"invulnerable_time",0.0f)/1000.f;
}

void CEntityCondition::reinit	()
{
	m_iLastTimeCalled		= 0;
	m_bTimeValid			= false;

	max_health()			= MAX_HEALTH;
	m_fPowerMax				= MAX_POWER;
	m_fRadiationMax			= MAX_RADIATION;
	m_fPsyHealthMax			= MAX_PSY_HEALTH;

	m_fEntityMorale			=  m_fEntityMoraleMax = 1.f;

	SetHealth				( MAX_HEALTH );
	m_fPower				= MAX_POWER;
	m_fRadiation			= 0;
	m_fPsyHealth			= MAX_PSY_HEALTH;

	m_fDeltaHealth			= 0;
	m_fDeltaPower			= 0;
	m_fDeltaRadiation		= 0;

	m_fDeltaCircumspection	= 0;
	m_fDeltaEntityMorale	= 0;
	m_fDeltaPsyHealth		= 0;

	m_fHealthLost			= 0.f;
	m_pWho					= NULL;
	m_iWhoID				= NULL;

	ClearWounds				();

}

void CEntityCondition::ChangeHealth(const float value)
{
	VERIFY(_valid(value));	
	m_fDeltaHealth += (CanBeHarmed() || (value > 0)) ? value : 0;
}

void CEntityCondition::ChangePower(const float value)
{
	m_fDeltaPower += value;
}

void CEntityCondition::ChangeRadiation(const float value)
{
	m_fDeltaRadiation += value;
}

void CEntityCondition::ChangePsyHealth(const float value)
{
	m_fDeltaPsyHealth += value;
}

void CEntityCondition::ChangeCircumspection(const float value)
{
	m_fDeltaCircumspection += value;
}
void CEntityCondition::ChangeEntityMorale(const float value)
{
	m_fDeltaEntityMorale += value;
}


void CEntityCondition::ChangeBleeding(const float percent)
{
	//затянуть раны
	for(WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
	{
		(*it)->Incarnation			(percent, m_fMinWoundSize);
		if(0 == (*it)->TotalSize	())
			(*it)->SetDestroy		(true);
	}
}

bool RemoveWoundPred(CWound* pWound)
{
	if(pWound->GetDestroy())
	{
		xr_delete		(pWound);
		return			true;
	}
	return				false;
}

void  CEntityCondition::UpdateWounds		()
{
	//убрать все зашившие раны из списка
	m_WoundVector.erase(
		std::remove_if(
			m_WoundVector.begin(),
			m_WoundVector.end(),
			&RemoveWoundPred
		),
		m_WoundVector.end()
	);
}

void CEntityCondition::UpdateConditionTime()
{
	u64 _cur_time = Level().GetGameTime();
	
	if(m_bTimeValid)
	{
		if (_cur_time > m_iLastTimeCalled){
			float x					= float(_cur_time-m_iLastTimeCalled)/1000.0f;
			SetConditionDeltaTime	(x);

		}else 
			SetConditionDeltaTime(0.0f);
	}
	else
	{
		SetConditionDeltaTime	(0.0f);
		m_bTimeValid			= true;

		m_fDeltaHealth			= 0;
		m_fDeltaPower			= 0;
		m_fDeltaRadiation		= 0;
		m_fDeltaCircumspection	= 0;
		m_fDeltaEntityMorale	= 0;
	}

	m_iLastTimeCalled			= _cur_time;
}

//вычисление параметров с ходом игрового времени
void CEntityCondition::UpdateCondition()
{
	if(GetHealth()<=0)			return;
	//-----------------------------------------
	bool CriticalHealth			= false;

	if (m_fDeltaHealth+GetHealth() <= 0)
	{
		CriticalHealth			= true;
		m_object->OnCriticalHitHealthLoss();
	}
	else
	{
		if (m_fDeltaHealth<0) m_object->OnHitHealthLoss(GetHealth()+m_fDeltaHealth);
	}
	//-----------------------------------------
	UpdateHealth				();
	//-----------------------------------------
	if (!CriticalHealth && m_fDeltaHealth+GetHealth() <= 0)
	{
		CriticalHealth			= true;
		m_object->OnCriticalWoundHealthLoss();
	};
	//-----------------------------------------
	UpdatePower					();
	UpdateRadiation				();
	//-----------------------------------------
	if (!CriticalHealth && m_fDeltaHealth+GetHealth() <= 0)
	{
		CriticalHealth = true;
		m_object->OnCriticalRadiationHealthLoss();
	};
	//-----------------------------------------
	UpdatePsyHealth				();

	UpdateEntityMorale			();

	if(Device.fTimeGlobal>m_fInvulnerableTime)
	{
		float curr_health			= GetHealth();
		if(curr_health>m_fKillHitTreshold && curr_health+m_fDeltaHealth<0)
		{
			SetHealth(m_fLastChanceHealth);
			m_fInvulnerableTime = Device.fTimeGlobal + m_fInvulnerableTimeDelta;
		}
		else
			SetHealth				( curr_health + m_fDeltaHealth );
	}

	m_fPower					+= m_fDeltaPower;
	m_fPsyHealth				+= m_fDeltaPsyHealth;
	m_fEntityMorale				+= m_fDeltaEntityMorale;
	m_fRadiation				+= m_fDeltaRadiation;
	
	m_fDeltaHealth				= 0;
	m_fDeltaPower				= 0;
	m_fDeltaRadiation			= 0;
	m_fDeltaPsyHealth			= 0;
	m_fDeltaCircumspection		= 0;
	m_fDeltaEntityMorale		= 0;
	float	l_health			= GetHealth() ;
	clamp						(l_health,			MIN_HEALTH, max_health());
	SetHealth					(l_health);
	clamp						(m_fPower,			0.0f,		m_fPowerMax);
	clamp						(m_fRadiation,		0.0f,		m_fRadiationMax);
	clamp						(m_fEntityMorale,	0.0f,		m_fEntityMoraleMax);
	clamp						(m_fPsyHealth,		0.0f,		m_fPsyHealthMax);
}

CCustomOutfit* CEntityCondition::GetOutfit()
{
	CInventoryOwner* pInvOwner	= smart_cast<CInventoryOwner*>(m_object);
	return						(pInvOwner) ? pInvOwner->GetOutfit() : false;
}

CHelmet* CEntityCondition::GetHelmet()
{
	CInventoryOwner* pInvOwner	= smart_cast<CInventoryOwner*>(m_object);
	return						(pInvOwner) ? pInvOwner->GetHelmet() : false;
}

template <typename T>
float CEntityCondition::GearExplEffect(T gear, float damage, float protection, CInventoryOwner* io, bool head)
{
	damage								*= head ? HitTypeHeadPart[ALife::eHitTypeExplosion] : 1.f - HitTypeHeadPart[ALife::eHitTypeExplosion];
	if (fEqual(damage, 0.f))
		return							damage;

	if (gear)
		protection						+= gear->GetBoneArmor(0);
	float armor_damage					= damage * ArmorDamageResistance.Calc(protection);
	if (gear)
		gear->Hit						(armor_damage, ALife::eHitTypeExplosion);
	io->HitArtefacts					(armor_damage, ALife::eHitTypeExplosion);

	return								damage * ExplDamageResistance.Calc(protection);
}

template <typename T>
float CEntityCondition::GearProtectionEffect(T gear, const ALife::EHitType& hit_type, float damage, bool head)
{
	damage								*= (head) ? HitTypeHeadPart[hit_type] : 1.f - HitTypeHeadPart[hit_type];
	if (!gear || fEqual(damage, 0.f))
		return							damage;

	float protection					= gear->GetHitTypeProtection(hit_type);
	gear->Hit							(damage * ProtectionDamageResistance.Calc(protection), hit_type);
	damage								*= AnomalyDamageResistance.Calc(protection);
	damage								-= min(damage, AnomalyDamageThreshold.Calc(protection));

	return								damage;
}

void CEntityCondition::HitProtectionEffect(SHit* pHDS)
{
	float main_damage[3]				= { pHDS->main_damage, pHDS->main_damage };
	float pierce_damage[2]				= { pHDS->pierce_damage, pHDS->pierce_damage };

	CCustomOutfit* outfit				= nullptr;
	CHelmet* helmet						= nullptr;
	float protection					= m_object->GetProtection(outfit, helmet, pHDS->boneID, pHDS->hit_type);
	CInventoryOwner* io					= smart_cast<CInventoryOwner*>(m_object);

	if (pHDS->DamageType() == 1)
	{
		if (pHDS->hit_type != ALife::eHitTypeRadiationGamma)
		{
			pHDS->pierce_hit_type		= pHDS->hit_type;
			pHDS->hit_type				= ALife::eHitTypeStrike;
		
			if (pHDS->pierce_hit_type == ALife::eHitTypeWound || pHDS->pierce_hit_type == ALife::eHitTypeWound_2)
			{
				float ap				= pHDS->main_damage * (pHDS->pierce_hit_type == ALife::eHitTypeWound_2 ? 20.f : 10.f);
				float k_speed_in		= (ap >= protection) ? sqrt(1.f - Level().BulletManager().m_fBulletAPLossOnPierce * protection / ap) : 0.f;
				if (fMore(k_speed_in, 0.f))
				{
					float full_damage	= pHDS->main_damage * m_fMeleeOnPierceDamageMultiplier;
					pHDS->main_damage	*= 1.f - k_speed_in;
					pHDS->pierce_damage	= full_damage - pHDS->main_damage;
					pHDS->armor_pierce_damage = pHDS->pierce_damage * m_fMeleeOnPierceArmorDamageFactor;
				}
			}
		
			if (io)
			{
				float armor_damage		= pHDS->main_damage * ArmorDamageResistance.Calc(protection);
				if (outfit)
				{
					outfit->Hit			(armor_damage, pHDS->hit_type);
					outfit->Hit			(pHDS->armor_pierce_damage, pHDS->pierce_hit_type);
				}
				else if (helmet)
				{
					helmet->Hit			(armor_damage, pHDS->hit_type);
					helmet->Hit			(pHDS->armor_pierce_damage, pHDS->pierce_hit_type);
				}
				io->HitArtefacts		(armor_damage, pHDS->hit_type);
			}
		
			pHDS->main_damage			*= m_fArmorDamageBoneScale;
			pHDS->pierce_damage			*= m_fPierceDamageBoneScale;

			main_damage[1]				= pHDS->main_damage;
			pierce_damage[1]			= pHDS->pierce_damage;
		}

		pHDS->main_damage				-= min(pHDS->main_damage, StrikeDamageThreshold.Calc(protection));
		if (fMore(pHDS->main_damage, 0.f))
			pHDS->main_damage			/= (1.f + StrikeDamageResistance.Calc(protection));
	}
	else if (pHDS->hit_type == ALife::eHitTypeExplosion)
	{
		if (m_object->scast<CBaseMonster*>())
			pHDS->main_damage			*= ExplDamageResistance.Calc(protection);
		else if (io)
		{
			float outfit_part			= GearExplEffect(outfit, pHDS->main_damage, protection, io, false);
			float helmet_part			= (outfit && !outfit->bIsHelmetAvaliable) ?
				GearExplEffect(outfit, pHDS->main_damage, protection, io, true) :
				GearExplEffect(helmet, pHDS->main_damage, protection, io, true);
			pHDS->main_damage			= outfit_part + helmet_part;
		}
	}
	else if (io)
	{
		if (fMore(protection, 0.f))
		{
			io->HitArtefacts			(pHDS->main_damage * ProtectionDamageResistance.Calc(protection), pHDS->hit_type);
			pHDS->main_damage			*= AnomalyDamageResistance.Calc(protection);
			pHDS->main_damage			-= min(pHDS->main_damage, AnomalyDamageThreshold.Calc(protection));
		}

		if (fMore(pHDS->main_damage, 0.f))
		{
			float outfit_part			= GearProtectionEffect(outfit, pHDS->hit_type, pHDS->main_damage, false);
			float helmet_part			= (outfit && !outfit->bIsHelmetAvaliable) ? GearProtectionEffect(outfit, pHDS->hit_type, pHDS->main_damage, true) : GearProtectionEffect(helmet, pHDS->hit_type, pHDS->main_damage, true);
			pHDS->main_damage			= outfit_part + helmet_part;
		}
	}
	main_damage[2]						= pHDS->main_damage;

	pHDS->main_damage					*= GetHitImmunity(pHDS->hit_type) * m_fHealthHitPart;
	if (pHDS->pierce_damage > 0.f)
		pHDS->pierce_damage				*= GetHitImmunity(pHDS->pierce_hit_type) * m_fHealthHitPart;

	if (Core.ParamFlags.test(Core.dbgcondfull) || Core.ParamFlags.test(Core.dbgcond) && (m_object->cast_actor() || m_pWho == Actor()))
		Msg("--xd CEntityCondition::HitProtectionEffect health [%.4f] protection [%.4f] main_damage [%.4f-%.4f-%.4f-%.4f] pierce_damage [%.4f-%.4f-%.4f]",
			GetHealth(), protection, main_damage[0], main_damage[1], main_damage[2], pHDS->main_damage, pierce_damage[0], pierce_damage[1], pHDS->pierce_damage);
}

float CEntityCondition::HitPowerEffect(float power_loss)
{
	return power_loss;
}

CWound* CEntityCondition::AddWound(float hit_power, ALife::EHitType hit_type, u16 element)
{
	//максимальное число косточек 64
	VERIFY(element  < 64 || BI_NONE == element);

	//запомнить кость по которой ударили и силу удара
	WOUND_VECTOR_IT it = m_WoundVector.begin();
	for(;it != m_WoundVector.end(); it++)
	{
		if((*it)->GetBoneNum() == element)
			break;
	}
	
	CWound* pWound = NULL;

	//новая рана
	if (it == m_WoundVector.end())
	{
		pWound = xr_new<CWound>(element);
		pWound->AddHit(hit_power*::Random.randF(0.5f,1.5f), hit_type);
		m_WoundVector.push_back(pWound);
	}
	//старая 
	else
	{
		pWound = *it;
		pWound->AddHit(hit_power*::Random.randF(0.5f,1.5f), hit_type);
	}

	VERIFY(pWound);
	return pWound;
}

CWound* CEntityCondition::ConditionHit(SHit* pHDS)
{
	m_pWho = pHDS->who;
	m_iWhoID = (m_pWho) ? m_pWho->ID() : 0;
	if (!CanBeHarmed())
		return nullptr;

	pHDS->main_damage *= HitTypeScale[pHDS->hit_type];
	HitProtectionEffect(pHDS);

	float d_neural{ pHDS->main_damage };
	float d_outer{ pHDS->main_damage }; 
	float d_inner{ pHDS->main_damage };
	float d_radiation{ 0.F };

	switch(pHDS->hit_type)
	{
	case ALife::eHitTypeBurn:
	case ALife::eHitTypeChemicalBurn:
		d_outer *= 1.25F;
		break;
	case ALife::eHitTypeLightBurn:
	case ALife::eHitTypeShock:
		d_outer *= .25F;
		break;
	case ALife::eHitTypeRadiation:
		d_neural = d_outer = d_inner = 0.F;
		d_radiation = pHDS->main_damage;
		break;
	case ALife::eHitTypeTelepatic:
		d_outer = d_inner = 0.F;
		break;
	case ALife::eHitTypeExplosion:
		d_outer *= .5F;
		break;
	case ALife::eHitTypeStrike:
		d_outer *= 0.75F;
		break;
	case ALife::eHitTypeRadiationGamma:
		d_neural = d_outer = 0.F;
		break;
	default:
		R_ASSERT2(0, "unknown hit type");
		break;
	}

	if (fMore(pHDS->pierce_damage, 0.F))
	{
		d_neural	+= pHDS->pierce_damage;
		d_outer		+= pHDS->pierce_damage;
		d_inner		+= pHDS->pierce_damage;
	}

	if (m_object->cast_actor())
	{
		luabind::functor<void>funct;
		ai().script_engine().functor("actor_condition_manager.process_hit", funct);
		funct(d_neural, d_outer, d_inner, d_radiation);
	}
	else
	{
		m_fHealthLost = d_neural;
		m_fDeltaHealth -= m_fHealthLost;
	}

	//раны добавляются только живому
	return /*(add_wound && (GetHealth() > 0)) ? AddWound(hit_power, pHDS->hit_type, pHDS->boneID) : */NULL;
}

float CEntityCondition::BleedingSpeed()
{
	float bleeding_speed		=0;

	for(WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; ++it)
		bleeding_speed			+= (*it)->TotalSize();
	
	return (m_WoundVector.empty() ? 0.f : bleeding_speed / m_WoundVector.size());
}

void CEntityCondition::UpdateHealth()
{
	float bleeding_speed		= BleedingSpeed() * m_fDeltaTime * m_change_v.m_fV_Bleeding;
	m_bIsBleeding				= fis_zero(bleeding_speed)?false:true;
	m_fDeltaHealth				-= CanBeHarmed() ? bleeding_speed : 0;
	m_fDeltaHealth				+= m_fDeltaTime * m_change_v.m_fV_HealthRestore;
	
	VERIFY						(_valid(m_fDeltaHealth));
	ChangeBleeding				(m_change_v.m_fV_WoundIncarnation * m_fDeltaTime);
}

void CEntityCondition::UpdatePower()
{
}

void CEntityCondition::UpdatePsyHealth()
{
	m_fDeltaPsyHealth += m_change_v.m_fV_PsyHealth*m_fDeltaTime;
}

void CEntityCondition::UpdateRadiation()
{
	if(m_fRadiation>0)
	{
		m_fDeltaRadiation -= m_change_v.m_fV_Radiation*
							m_fDeltaTime;

		m_fDeltaHealth -= CanBeHarmed() ? m_change_v.m_fV_RadiationHealth*m_fRadiation*m_fDeltaTime : 0.0f;
	}
}

void CEntityCondition::UpdateEntityMorale()
{
	if(m_fEntityMorale<m_fEntityMoraleMax)
	{
		m_fDeltaEntityMorale += m_change_v.m_fV_EntityMorale*m_fDeltaTime;
	}
}


bool CEntityCondition::IsLimping() const
{
	if (!m_use_limping_state)
		return	(false);
	return (m_fPower*GetHealth() <= m_limping_threshold);
}

void CEntityCondition::save	(NET_Packet &output_packet)
{
	u8 is_alive	= (GetHealth()>0.f)?1:0;
	
	output_packet.w_u8	(is_alive);
	if(is_alive)
	{
		save_data						(m_fPower,output_packet);
		save_data						(m_fRadiation,output_packet);
		save_data						(m_fEntityMorale,output_packet);
		save_data						(m_fPsyHealth,output_packet);

		output_packet.w_u8				((u8)m_WoundVector.size());
		for(WOUND_VECTOR_IT it = m_WoundVector.begin(); m_WoundVector.end() != it; it++)
			(*it)->save(output_packet);
	}
}

void CEntityCondition::load	(IReader &input_packet)
{
	m_bTimeValid = false;

	u8 is_alive				= input_packet.r_u8	();
	if(is_alive)
	{
		load_data						(m_fPower,input_packet);
		load_data						(m_fRadiation,input_packet);
		load_data						(m_fEntityMorale,input_packet);
		load_data						(m_fPsyHealth,input_packet);

		ClearWounds();
		m_WoundVector.resize(input_packet.r_u8());
		if(!m_WoundVector.empty())
			for(u32 i=0; i<m_WoundVector.size(); i++)
			{
				CWound* pWound = xr_new<CWound>(BI_NONE);
				pWound->load(input_packet);
				m_WoundVector[i] = pWound;
			}
	}
}

void CEntityCondition::SConditionChangeV::load(LPCSTR sect, LPCSTR prefix)
{
	string256				str;
	m_fV_Circumspection		= 0.01f;

	strconcat				(sizeof(str),str,"radiation_v",prefix);
	m_fV_Radiation			= pSettings->r_float(sect,str);
	strconcat				(sizeof(str),str,"radiation_health_v",prefix);
	m_fV_RadiationHealth	= pSettings->r_float(sect,str);
	strconcat				(sizeof(str),str,"morale_v",prefix);
	m_fV_EntityMorale		= pSettings->r_float(sect,str);
	strconcat				(sizeof(str),str,"psy_health_v",prefix);
	m_fV_PsyHealth			= pSettings->r_float(sect,str);	
	strconcat				(sizeof(str),str,"bleeding_v",prefix);
	m_fV_Bleeding			= pSettings->r_float(sect,str);
	strconcat				(sizeof(str),str,"wound_incarnation_v",prefix);
	m_fV_WoundIncarnation	= pSettings->r_float(sect,str);
	strconcat				(sizeof(str),str,"health_restore_v",prefix);
	m_fV_HealthRestore		= READ_IF_EXISTS(pSettings,r_float,sect, str,0.0f);
}

void CEntityCondition::remove_links	(const CObject *object)
{
	if (m_pWho != object)
		return;

	m_pWho					= m_object;
	m_iWhoID				= m_object->ID();
}

bool CEntityCondition::ApplyInfluence(const SMedicineInfluenceValues& V, const shared_str& sect)
{
	ChangeHealth	(V.fHealth);
	ChangePower		(V.fPower);
	ChangeSatiety	(V.fSatiety);
	ChangeRadiation	(V.fRadiation);
	ChangeBleeding	(V.fWoundsHeal);
	SetMaxPower		(GetMaxPower()+V.fMaxPowerUp);
	ChangeAlcohol	(V.fAlcohol);
	return true;
}

bool CEntityCondition::ApplyBooster(const SBooster& B, const shared_str& sect)
{
	return true;
}

void SMedicineInfluenceValues::Load(const shared_str& sect, float depletion_rate)
{
	fHealth			= depletion_rate * pSettings->r_float(sect.c_str(), "eat_health");
	fPower			= depletion_rate * pSettings->r_float(sect.c_str(), "eat_power");
	fSatiety		= depletion_rate * pSettings->r_float(sect.c_str(), "eat_satiety");
	fRadiation		= depletion_rate * pSettings->r_float(sect.c_str(), "eat_radiation");
	fWoundsHeal		= depletion_rate * pSettings->r_float(sect.c_str(), "wounds_heal_perc");
	clamp			(fWoundsHeal, 0.f, 1.f);
	fMaxPowerUp		= depletion_rate * READ_IF_EXISTS	(pSettings,r_float,sect.c_str(),	"eat_max_power",0.0f);
	fAlcohol		= depletion_rate * READ_IF_EXISTS	(pSettings, r_float, sect.c_str(),	"eat_alcohol", 0.0f);
	fTimeTotal		= depletion_rate * READ_IF_EXISTS	(pSettings, r_float, sect.c_str(),	"apply_time_sec", -1.0f);
}

void SBooster::Load(const shared_str& sect, EBoostParams type)
{
	fBoostTime		= pSettings->r_float(sect, "boost_time");
	fBoostValue		= pSettings->r_float(sect, ef_boosters_section_names[type]);
	m_type			= type;
}

void CEntityCondition::loadStaticData()
{
	using namespace ALife;

	HitTypeHeadPart.resize				(eHitTypeMax);
	for (int i = 0; i < eHitTypeMax; i++)
		HitTypeHeadPart[i]				= 0.f;
	HitTypeHeadPart[eHitTypeBurn]		= pSettings->r_float("hit_type_head_part", "burn");
	HitTypeHeadPart[eHitTypeShock]		= pSettings->r_float("hit_type_head_part", "shock");
	HitTypeHeadPart[eHitTypeRadiation]	= pSettings->r_float("hit_type_head_part", "radiation");
	HitTypeHeadPart[eHitTypeChemicalBurn] = pSettings->r_float("hit_type_head_part", "chemical_burn");
	HitTypeHeadPart[eHitTypeTelepatic]	= pSettings->r_float("hit_type_head_part", "telepatic");
	HitTypeHeadPart[eHitTypeLightBurn]	= HitTypeHeadPart[eHitTypeBurn];
	HitTypeHeadPart[eHitTypeExplosion]	= pSettings->r_float("hit_type_head_part", "explosion");
	
	HitTypeScale.resize					(eHitTypeMax);
	HitTypeScale[eHitTypeBurn]			= pSettings->r_float("hit_type_global_scale", "burn");
	HitTypeScale[eHitTypeShock]			= pSettings->r_float("hit_type_global_scale", "shock");
	HitTypeScale[eHitTypeChemicalBurn]	= pSettings->r_float("hit_type_global_scale", "chemical_burn");
	HitTypeScale[eHitTypeRadiation]		= pSettings->r_float("hit_type_global_scale", "radiation");
	HitTypeScale[eHitTypeTelepatic]		= pSettings->r_float("hit_type_global_scale", "telepatic");
	HitTypeScale[eHitTypeWound]			= pSettings->r_float("hit_type_global_scale", "wound");
	HitTypeScale[eHitTypeFireWound]		= pSettings->r_float("hit_type_global_scale", "fire_wound");
	HitTypeScale[eHitTypeStrike]		= pSettings->r_float("hit_type_global_scale", "strike");
	HitTypeScale[eHitTypeExplosion]		= pSettings->r_float("hit_type_global_scale", "explosion");
	HitTypeScale[eHitTypeWound_2]		= pSettings->r_float("hit_type_global_scale", "wound_2");
	HitTypeScale[eHitTypeLightBurn]		= pSettings->r_float("hit_type_global_scale", "light_burn");
	HitTypeScale[eHitTypeRadiationGamma]= pSettings->r_float("hit_type_global_scale", "radiation_gamma");
	
	m_fMeleeOnPierceDamageMultiplier	= pSettings->r_float("damage_manager", "melee_on_pierce_damage_multiplier");
	m_fMeleeOnPierceArmorDamageFactor	= pSettings->r_float("damage_manager", "melee_on_pierce_armor_damage_factor");

	StrikeDamageThreshold.Load			("damage_manager", "strike_damage_threshold");
	StrikeDamageResistance.Load			("damage_manager", "strike_damage_resistance");
	ExplDamageResistance.Load			("damage_manager", "expl_damage_resistance");
	ArmorDamageResistance.Load			("damage_manager", "armor_damage_resistance");
	
	AnomalyDamageThreshold.Load			("damage_manager", "anomaly_damage_threshold");
	AnomalyDamageResistance.Load		("damage_manager", "anomaly_damage_resistance");
	ProtectionDamageResistance.Load		("damage_manager", "protection_damage_resistance");
}
