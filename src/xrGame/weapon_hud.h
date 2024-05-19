#pragma once
#include "Weapon.h"

class CGrenadeLauncher;
class CScope;
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
	enum EHandsOffset
	{
		eRelaxed,
		eArmed,
		eAim,
		eIS,
		eAlt,
		eGL,
		eTotal
	};

private:
	CWeaponMagazined&					O;

public:
										CWeaponHud								(CWeaponMagazined* obj);

private:
	float								m_fRotationFactor						= 0.f;
	bool								m_going_to_fire							= false;
	Dvector								m_current_hud_offset[2]					= { dZero, dZero };

	Dvector								m_hud_offset[eTotal][2];
	float								m_fRotateTime;
	Dvector								m_grip_offset;

	EHandsOffset						get_target_hud_offset_idx			C$	();
	Dvector CP$							get_target_hud_offset				C$	();

public:
	static SPowerDependency				HandlingToRotationTime;

	void								InitRotateTime							(float cif);
	void								UpdateHudAdditional						(Dmatrix& trans);
	bool								Action									(u16 cmd, u32 flags);
	void								ProcessGL								(CGrenadeLauncher* gl);

	bool								IsRotatingToZoom					C$	();
	Fvector								getMuzzleSightOffset				C$	();
	void								ProcessScope						C$	(CScope* scope, bool attach);
};
