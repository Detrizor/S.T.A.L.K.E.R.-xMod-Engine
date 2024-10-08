#pragma once
#include "Weapon.h"

class MGrenadeLauncher;
class MScope;
enum eScopeType;

struct SShootingParams
{
	Fvector4							m_shot_max_offset_LRUD;
	float								m_shot_offset_BACKW;
	float								m_ret_speed;
	float								m_min_LRUD_power;
};

class CWeaponHud
{
	CWeaponMagazined&					O;
	enum EHandsOffset
	{
		eRelaxed,
		eArmed,
		eAim,
		eAlt,
		eGL,
		eTotal
	};

public:
										CWeaponHud								(CWeaponMagazined* obj);

private:
	static float						s_recoil_hud_angle_per_shift;
	static float						s_recoil_hud_roll_per_shift;
	static float						s_recoil_hud_rollback_per_shift;
	static float						s_max_rotate_speed;
	static float						s_rotate_accel_time;
	static float						s_iron_sights_max_angle;
	
	float								m_aim_z_offset;

	float								m_fRotationFactor						= 0.f;
	bool								m_going_to_fire							= false;
	Dvector								m_current_hud_offset[2]					= { dZero, dZero };
	Dvector								m_current_d_rot							= dZero;
	float								m_current_rotate_speed					= 0.f;
	Dvector CP$							m_prev_offset							= nullptr;

	Dvector								m_hud_offset[eTotal][2];
	float								m_fRotateTime;
	Dvector								m_grip_offset;

	EHandsOffset						get_target_hud_offset_idx			C$	();
	Dvector CP$							get_target_hud_offset				C$	();

public:
	static void							loadStaticData							();

	void								UpdateHudAdditional						(Dmatrix& trans);
	bool								Action									(u16 cmd, u32 flags);
	void								ProcessGL								(MGrenadeLauncher* gl);
	void								calculateAimOffsets						();

	bool								IsRotatingToZoom					C$	();
	Fvector								getTransference						C$	(float distance);
	void								calculateScopeOffset				C$	(MScope* scope);
};
