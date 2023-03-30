#pragma once
#include "Weapon.h"

enum EHandsOffset
{
	eRelaxed,
	eArmed,
	eAim,
	eADS,
	eAlt,
	eGL,
	eTotal
};

struct SShootingParams
{
	bool								bShootShake;
	Fvector4							m_shot_max_offset_LRUD;
	Fvector4							m_shot_max_offset_LRUD_aim;
	Fvector2							m_shot_offset_BACKW;
	float								m_ret_speed;
	float								m_ret_speed_aim;
	float								m_min_LRUD_power;
};

class CWeaponHud
{
private:
	CWeaponMagazined PC$				pO;
	CWeaponMagazined&					O;

public:
										CWeaponHud								(CWeaponMagazined* obj);

private:
	EHandsOffset						last_idx;
	float								m_lense_offset;
	float								m_fRotateTime;
	float								m_fRelaxTime;
	float								m_fRotationFactor;
	float								m_fLR_ShootingFactor; // Фактор горизонтального сдвига худа при стрельбе [-1; +1]
	float								m_fUD_ShootingFactor; // Фактор вертикального сдвига худа при стрельбе [-1; +1]
	float								m_fBACKW_ShootingFactor; // Фактор сдвига худа в сторону лица при стрельбе [0; +1]
	Fvector								m_hands_offset[2][eTotal];
	Fvector								m_hud_offset[2];
	SShootingParams						m_shooting_params;
	SafemodeAnm							m_safemode_anm[2];
	Fmatrix								m_shoot_shake_mat;

public:
	SPowerDependency				S$	HandlingToRotationTime;
	SPowerDependency				S$	HandlingToRelaxTime;

	Fmatrix CR$							shoot_shake_mat						C$	()		{ return m_shoot_shake_mat; }

	void								InitRotateTime							(float cif);
	void								UpdateHudAdditional						(Fmatrix& trans);
	bool								Action									(u16 cmd, u32 flags);

	EHandsOffset						GetCurrentHudOffsetIdx				C$	();
	bool								IsRotatingToZoom					C$	();
	bool								ReadyToFire							C$	();
	//type_name						V$	method_name							C$	()
};
