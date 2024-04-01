#pragma once
#include "Weapon.h"

struct SAddonSlot;
enum eScopeType;

enum EHandsOffset
{
	eRelaxed,
	eArmed,
	eAim,
	eIS,
	eScope,
	eAlt,
	eScopeAlt,
	eGL,
	eTotal
};

struct SShootingParams
{
	Fvector4							m_shot_max_offset_LRUD;
	float								m_shot_offset_BACKW;
	float								m_ret_speed;
	float								m_min_LRUD_power;
};

class CWeaponHud
{
private:
	CWeaponMagazined&					O;

public:
										CWeaponHud								(CWeaponMagazined* obj);

private:
	float								m_lense_offset;
	float								m_fRotateTime;
	float								m_fRotationFactor;
	float								m_fLR_ShootingFactor; // Фактор горизонтального сдвига худа при стрельбе [-1; +1]
	float								m_fUD_ShootingFactor; // Фактор вертикального сдвига худа при стрельбе [-1; +1]
	float								m_fBACKW_ShootingFactor; // Фактор сдвига худа в сторону лица при стрельбе [0; +1]
	Fvector								m_hands_offset[eTotal][2];
	Fvector								m_hud_offset[2];
	Fvector								m_root_offset;
	Fvector								m_root_offset_gl;
	Fvector								m_barrel_offset;
	SafemodeAnm							m_safemode_anm[2];
	Fmatrix								m_shoot_shake_mat;
	Fvector								m_cur_offs;
	u8									m_scope;
	bool								m_alt_scope;
	bool								m_gl;
	bool								m_scope_alt_aim_via_iron_sights;
	bool								m_scope_own_alt_aim;
	bool								m_going_to_fire;

	void								CalcAimOffset							();

public:
	SPowerDependency				S$	HandlingToRotationTime;

	Fmatrix CR$							shoot_shake_mat						C$	()				{ return m_shoot_shake_mat; }
	Fvector CPC							HandsOffset							C$	(int idx)		{ return m_hands_offset[idx]; }
	Fvector CPC							HudOffset							C$	()				{ return m_hud_offset; }
	float								LenseOffset							C$	()				{ return m_lense_offset; }

	void								InitRotateTime							(float cif);
	void								UpdateHudAdditional						(Fmatrix& trans);
	bool								Action									(u16 cmd, u32 flags);
	void								ProcessScope							(SAddonSlot* slot, bool attach);
	void								ProcessGL								(SAddonSlot* slot, bool attach);
	void								SwitchGL								();

	EHandsOffset						GetCurrentHudOffsetIdx				C$	();
	bool								IsRotatingToZoom					C$	();
	Fvector								BarrelSightOffset					C$	();
};
