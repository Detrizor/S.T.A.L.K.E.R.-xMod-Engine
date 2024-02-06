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
//	m_first_shot_pos = 0.0f;
}

void CWeaponShotEffector::Initialize(CameraRecoil CR$ cam_recoil)
{
	m_cam_recoil = &cam_recoil;
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

void CWeaponShotEffector::Shot( CWeapon* weapon )
{
	R_ASSERT( weapon );
	weapon->updateCamRecoil();
	m_shot_numer = weapon->ShotsFired() - 1;
	if ( m_shot_numer <= 0 )
	{
		m_shot_numer = 0;
		Reset();
	}
	m_single_shot = (weapon->GetCurrentFireMode() == 1);

	float angle = m_cam_recoil->StepAngleVert;
	angle      += m_cam_recoil->StepAngleVertInc * (float)m_shot_numer;
	m_angle_vert += angle;

	clamp(m_angle_vert, -m_cam_recoil->MaxAngleVert, m_cam_recoil->MaxAngleVert);
	if (fis_zero(m_angle_vert - m_cam_recoil->MaxAngleVert))
		m_angle_vert *= m_Random.randF(0.96f, 1.04f);

	angle = m_cam_recoil->StepAngleHorz;
	angle += m_cam_recoil->StepAngleHorzInc * (float)m_shot_numer;
	m_angle_horz += angle * m_Random.randF(-1.f, 1.f);

	clamp(m_angle_horz, -m_cam_recoil->MaxAngleHorz, m_cam_recoil->MaxAngleHorz);
	if (fis_zero(m_angle_horz - m_cam_recoil->MaxAngleHorz))
		m_angle_horz *= m_Random.randF(0.96f, 1.04f);

	m_first_shot = true;
	m_actived = true;
	m_shot_end = false;
}

void CWeaponShotEffector::Shot2( float angle )
{
	m_angle_vert += angle;

	clamp( m_angle_vert, -m_cam_recoil->MaxAngleVert, m_cam_recoil->MaxAngleVert );
	if ( fis_zero(m_angle_vert - m_cam_recoil->MaxAngleVert) )
	{
		m_angle_vert *= m_Random.randF( 0.96f, 1.04f );
	}
	
	if (m_Random.randF(0.f,1.f) < 0.5f)
		m_angle_horz -= m_cam_recoil->StepAngleHorz;

	clamp( m_angle_horz, -m_cam_recoil->MaxAngleHorz, m_cam_recoil->MaxAngleHorz );

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

CCameraShotEffector::CCameraShotEffector(CameraRecoil CR$ cam_recoil)
 : CEffectorCam(eCEShot,100000.0f)
{
	CWeaponShotEffector::Initialize(cam_recoil);
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
