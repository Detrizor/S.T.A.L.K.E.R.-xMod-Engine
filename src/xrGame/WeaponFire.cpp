// WeaponFire.cpp: implementation of the CWeapon class.
// function responsible for firing with CWeapon
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity.h"
#include "actor.h"

#include "actoreffector.h"
#include "effectorshot.h"

#include "level_bullet_manager.h"

#include "game_cl_mp.h"
#include "reward_event_generator.h"

#define FLAME_TIME 0.05f


float _nrand(float sigma)
{
#define ONE_OVER_SIGMA_EXP (1.0f / 0.7975f)

	if(sigma == 0) return 0;

	float y;
	do{
		y = -logf(Random.randF());
	}while(Random.randF() > expf(-_sqr(y - 1.0f)*0.5f));
	if(rand() & 0x1)	return y * sigma * ONE_OVER_SIGMA_EXP;
	else				return -y * sigma * ONE_OVER_SIGMA_EXP;
}

void random_dir(Fvector& tgt_dir, const Fvector& src_dir, float dispersion)
{
	float sigma			= dispersion/3.f;
	float alpha			= clampr		(_nrand(sigma),-dispersion,dispersion);
	float theta			= Random.randF	(0,PI);
	float r 			= tan			(alpha);
	Fvector 			U,V,T;
	Fvector::generate_orthonormal_basis	(src_dir,U,V);
	U.mul				(r*_sin(theta));
	V.mul				(r*_cos(theta));
	T.add				(U,V);
	tgt_dir.add			(src_dir,T).normalize();
}

float CWeapon::GetWeaponDeterioration	()
{
	return conditionDecreasePerShot;
};

void CWeapon::FireTrace		()
{
	CCartridge l_cartridge = getCartridgeToShoot();
	VERIFY		(u16(-1) != l_cartridge.bullet_material_idx);

	ChangeCondition(-GetWeaponDeterioration()*l_cartridge.param_s.impair);

	Fvector p = get_LastFP();
	Fvector d = getFullFireDirection(l_cartridge);
	bool SendHit = SendHitAllowed(H_Parent());
	//выстерлить пулю (с учетом возможной стрельбы дробью)
	for (int i = 0; i < l_cartridge.param_s.buckShot; ++i)
		FireBullet(p, d, GetFireDispersion(&l_cartridge), l_cartridge, H_Parent()->ID(), ID(), SendHit);

	StartShotParticles		();
	
	if(m_bLightShotEnabled) 
		Light_Start			();

	appendRecoil			(m_fStartBulletSpeed * m_silencer_koef.bullet_speed * l_cartridge.param_s.fBulletMass * l_cartridge.param_s.buckShot);
}

void CWeapon::StopShooting()
{
//	SetPending			(TRUE);

	//принудительно останавливать зацикленные партиклы
	if(m_pFlameParticles && m_pFlameParticles->IsLooped())
		StopFlameParticles	();	

	SwitchState(eIdle);

	bWorking = false;
}

void CWeapon::FireEnd() 
{
	CShootingObject::FireEnd();
	StopShotEffector();
}

#define s_recoil_kick_weight pSettings->r_float("weapon_manager", "recoil_kick_weight")
#define s_recoil_tremble_weight pSettings->r_float("weapon_manager", "recoil_tremble_weight")
#define s_recoil_roll_weight pSettings->r_float("weapon_manager", "recoil_roll_weight")

#define s_recoil_tremble_mean_change_chance pSettings->r_float("weapon_manager", "recoil_tremble_mean_change_chance")
#define s_recoil_tremble_dispersion pSettings->r_float("weapon_manager", "recoil_tremble_dispersion")
#define s_recoil_kick_dispersion pSettings->r_float("weapon_manager", "recoil_kick_dispersion")
#define s_recoil_roll_dispersion pSettings->r_float("weapon_manager", "recoil_roll_dispersion")
void CWeapon::appendRecoil(float impulse_magnitude)
{
	Fvector pattern				= vZero;
	pattern.add					(m_stock_recoil_pattern);
	pattern.add					(m_layout_recoil_pattern);
	pattern.add					(m_mechanic_recoil_pattern);

	if ((ShotsFired() == 1) || (Random.randF() < s_recoil_tremble_mean_change_chance))
		m_recoil_tremble_mean	= Random.randFs(1.f);

	float tremble				= pattern.x * Random.randFs(s_recoil_tremble_dispersion, m_recoil_tremble_mean);
	float kick					= pattern.y * Random.randFs(s_recoil_kick_dispersion, 1.f);
	float roll					= pattern.z * Random.randFs(s_recoil_roll_dispersion);
	Fvector shot_impulse		= {
		tremble * s_recoil_tremble_weight,
		kick * s_recoil_kick_weight,
		roll * s_recoil_roll_weight
	};

	shot_impulse.mul			(impulse_magnitude);
	Fvector4 shot_impulse_full	= {
		shot_impulse.x,
		shot_impulse.y,
		shot_impulse.z,
		impulse_magnitude - shot_impulse.x - shot_impulse.y - shot_impulse.z
	};
	m_recoil_hud_impulse.add	(shot_impulse_full);
	m_recoil_cam_impulse.add	(shot_impulse);
	m_recoil_cam_last_impulse	= shot_impulse;
}

bool CWeapon::isCamRecoilRelaxed() const
{
	return (m_recoil_cam_last_impulse == vZero) && (m_recoil_cam_impulse == vZero);
}
