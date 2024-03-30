// EffectorShot.cpp: implementation of the CCameraShotEffector class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EffectorShot.h"
#include "Weapon.h"

//-----------------------------------------------------------------------------
// Weapon shot effector
//-----------------------------------------------------------------------------
CWeaponShotEffector::CWeaponShotEffector()
{
	Reset();
}

void CWeaponShotEffector::Initialize()
{
	Reset();
}

void CWeaponShotEffector::Reset()
{
	m_angle_vert	= 0.0f;
	m_angle_horz	= 0.0f;

	m_prev_angle_vert = 0.0f;
	m_prev_angle_horz = 0.0f;

	m_delta_vert	= 0.0f;
	m_delta_horz	= 0.0f;

	m_LastSeed		= 0;
	m_single_shot	= false;
	m_first_shot	= false;
	m_actived		= false;
	m_shot_end		= true;
}

#define s_recoil_angle_per_impulse pSettings->r_float("weapon_manager", "recoil_angle_per_impulse")
#define s_recoil_horz_angle_coeff pSettings->r_float("weapon_manager", "recoil_horz_angle_coeff")

void CWeaponShotEffector::Shot(CWeapon* weapon, float accuracy)
{
	R_ASSERT( weapon );
	m_shot_numer = weapon->ShotsFired() - 1;
	if ( m_shot_numer <= 0 )
	{
		m_shot_numer = 0;
		Reset();
	}
	m_single_shot = (weapon->GetCurrentFireMode() == 1);

	float coeff		= s_recoil_angle_per_impulse * accuracy / weapon->GetControlInertionFactor();

	float vangle	= weapon->getLastRecoilImpulseMagnitude();
	m_angle_vert	+= vangle * coeff;

	float hangle	= (m_first_shot) ? weapon->getLastRecoilImpulseMagnitude() : 0.f;
	hangle			*= s_recoil_horz_angle_coeff * m_Random.randF(-1.f, 1.f);
	m_angle_horz	+= hangle * coeff;

	m_first_shot	= true;
	m_actived		= true;
	m_shot_end		= false;

	weapon->setLastRecoilImpulse(vangle, hangle);
}

void CWeaponShotEffector::Shot2( float angle )
{
	m_angle_vert += angle;

	m_first_shot	= true;
	m_actived		= true;
	m_shot_end		= false;
}

void CWeaponShotEffector::Update()
{
	if (m_actived && m_shot_end && !m_single_shot)
		m_actived = false;

	m_delta_vert = m_angle_vert - m_prev_angle_vert;
	m_delta_horz = m_angle_horz - m_prev_angle_horz;
	
	m_prev_angle_vert = m_angle_vert; 
	m_prev_angle_horz = m_angle_horz;

//	Msg( " <<[%d]  v=%.4f  dv=%.4f   a=%d s=%d  fr=%d", m_shot_numer, m_angle_vert, m_delta_vert, m_actived, m_first_shot, Device.dwFrame );
}

void CWeaponShotEffector::GetDeltaAngle		(Fvector& angle)
{
	angle.x			= -m_angle_vert;
	angle.y			= -m_angle_horz;
	angle.z			= 0.0f;
}

void CWeaponShotEffector::GetLastDelta		(Fvector& delta_angle)
{
	delta_angle.x	= -m_delta_vert;
	delta_angle.y	= -m_delta_horz;
	delta_angle.z	= 0.0f;
}

void CWeaponShotEffector::SetRndSeed	(s32 Seed)
{
	if (m_LastSeed == 0)
	{
		m_LastSeed			= Seed;
//		m_Random.seed		(Seed);
		m_Random.seed		(Device.dwFrame);
	}
}

void CWeaponShotEffector::ChangeHP( float* pitch, float* yaw )
{
	*pitch -= m_delta_vert; // y = pitch = p = vert
	*yaw   -= m_delta_horz; // x = yaw   = h = horz
}

//-----------------------------------------------------------------------------
// Camera shot effector
//-----------------------------------------------------------------------------

CCameraShotEffector::CCameraShotEffector() : CEffectorCam(eCEShot,100000.0f)
{
	CWeaponShotEffector::Initialize();
	m_pActor		= NULL;
}

CCameraShotEffector::~CCameraShotEffector()
{
}

BOOL CCameraShotEffector::ProcessCam(SCamEffectorInfo& info)
{
	Update();
	return TRUE;
}
