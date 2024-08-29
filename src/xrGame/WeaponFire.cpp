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
}

void CWeapon::FireTrace()
{
	m_shot_cartridge					= getCartridgeToShoot();
	VERIFY								(m_shot_cartridge.bullet_material_idx != u16_max);

	Fvector p							= get_LastFP();
	Fvector d							= getFullFireDirection(m_shot_cartridge);
	float disp							= GetFireDispersion(&m_shot_cartridge);
	bool SendHit						= SendHitAllowed(H_Parent());

	//выстерлить пулю (с учетом возможной стрельбы дробью)
	for (int i = 0; i < m_shot_cartridge.param_s.buckShot; ++i)
		FireBullet						(p, d, disp, m_shot_cartridge, H_Parent()->ID(), ID(), SendHit);

	StartShotParticles					();
	Light_Start							();

	float shot_speed					= m_barrel_len * m_muzzle_koefs.bullet_speed * m_shot_cartridge.param_s.bullet_speed_per_barrel_len;
	float shot_mass						= m_shot_cartridge.param_s.fBulletMass * m_shot_cartridge.param_s.buckShot;
	appendRecoil						(shot_speed * shot_mass);

	ChangeCondition						(-GetWeaponDeterioration() * l_cartridge.param_s.impair);
}

void CWeapon::StopShooting()
{
	//принудительно останавливать зацикленные партиклы
	if (m_flame_particles && m_flame_particles->IsLooped())
		StopFlameParticles	();	

	SwitchState(eIdle);
	bWorking = false;
}

void CWeapon::FireEnd() 
{
	CShootingObject::FireEnd();
	StopShotEffector();
}

void CWeapon::appendRecoil(float impulse_magnitude)
{
	Fvector pattern				= m_mechanic_recoil_pattern;
	pattern.mul					(m_layout_recoil_pattern);
	pattern.mul					((IsZoomed()) ? m_stock_recoil_pattern : m_stock_recoil_pattern_absent);
	pattern.mul					(m_muzzle_recoil_pattern);
	pattern.mul					(m_foregrip_recoil_pattern);

	if ((ShotsFired() == 1) || (Random.randF() < s_recoil_tremble_mean_change_chance))
		m_recoil_tremble_mean	= Random.randFs(s_recoil_tremble_mean_dispersion);

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
