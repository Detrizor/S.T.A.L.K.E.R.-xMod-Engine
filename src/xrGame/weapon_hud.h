#pragma once
#include "Weapon.h"

class CGrenadeLauncher;
class CScope;
struct SAddonSlot;
enum eScopeType;

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
	float								m_fRotationFactor						= 0.f;
	bool								m_going_to_fire							= false;
	bool								m_gl									= false;
	Fvector								m_current_hud_offset[2]					= { vZero, vZero };
	xr_vector<CScope*>					m_scopes_to_process						= {};

	Fvector								m_barrel_offset;
	Fvector								m_hud_offset[eTotal][2];
	float								m_fRotateTime;

	void								calc_aim_offset							();
	void								process_scope_impl						(CScope* scope);

	EHandsOffset						get_target_hud_offset_idx			C$	();
	Fvector CP$							get_target_hud_offset				C$	();

public:
	static SPowerDependency				HandlingToRotationTime;

	void								InitRotateTime							(float cif);
	void								UpdateHudAdditional						(Fmatrix& trans);
	bool								Action									(u16 cmd, u32 flags);
	void								ProcessScope							(CScope* scope, bool attach);
	void								ProcessGL								(SAddonSlot* slot, CGrenadeLauncher* gl, bool attach);
	void								SwitchGL								();

	bool								IsRotatingToZoom					C$	();
	Fvector								getMuzzleSightOffset				C$	();
};
