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
	m_angle_roll	= 0.0f;

	m_delta_vert	= 0.0f;
	m_delta_horz	= 0.0f;
	m_delta_roll	= 0.0f;

	m_LastSeed		= 0;
	m_first_shot	= false;
	m_actived		= false;
	m_shot_end		= true;
}

void CWeaponShotEffector::Shot(CWeapon* weapon)
{
	R_ASSERT( weapon );
	m_shot_numer = weapon->ShotsFired() - 1;
	if (m_shot_numer <= 0)
	{
		m_shot_numer = 0;
		Reset();
	}

	m_first_shot	= true;
	m_actived		= true;
	m_shot_end		= false;

	m_weapon		= weapon;
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
	if (m_actived && m_weapon->isCamRecoilRelaxed())
		m_actived = false;

	auto delta = m_weapon->getRecoilCamDelta();
	m_delta_horz = delta.x;
	m_delta_vert = delta.y;
	//--xd problematic m_delta_roll = m_weapon->getRecoilShiftDelta().z * coeff;
	
	m_angle_vert += m_delta_vert;
	m_angle_horz += m_delta_horz;
	m_angle_roll += m_delta_roll;
}

void CWeaponShotEffector::GetDeltaAngle		(Fvector& angle)
{
	angle.x			= -m_angle_vert;
	angle.y			= -m_angle_horz;
	angle.z			= -m_angle_roll;
}

void CWeaponShotEffector::GetLastDelta		(Fvector& delta_angle)
{
	delta_angle.x	= -m_delta_vert;
	delta_angle.y	= -m_delta_horz;
	delta_angle.z	= -m_delta_roll;
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

void CWeaponShotEffector::ChangeHP(float& pitch, float& yaw, float& roll)
{
	pitch	-= m_delta_vert;
	yaw		-= m_delta_horz;
	roll	-= m_delta_roll;
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
