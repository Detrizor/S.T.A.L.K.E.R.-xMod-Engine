#pragma once
#include "inventory_item_object.h"
#include "anticheat_dumpable_object.h"
#include "script_export_space.h"

struct SCartridgeParam
{
	float								kDisp									= 1.f;
	float								barrel_length							= 1.f;
	float								barrel_len								= 1.f;
	float								bullet_speed_per_barrel_len				= 1.f;
	float								impair									= 1.f;
	float								fBulletMass								= 1.f;
	float								fBulletResist							= 1.f;
	float								penetration								= 1.f;
	float								fWMS									= -1.f;
	float								fAirResist								= 1.f;
	float								fAirResistZeroingCorrection				= 1000.f;
	float								bullet_k_ap								= 1.f;
	int									buckShot								= 1;
	u8									u8ColorID								= 0;
	bool								bullet_hollow_point						= false;
};

class CCartridge : public IAnticheatDumpable
{
public:
	enum
	{
		cfTracer						= (1<<0),
		cfRicochet						= (1<<1),
		cfExplosive						= (1<<2),
		cfMagneticBeam					= (1<<3),
	};

public:
										CCartridge								();
										CCartridge								(LPCSTR section);

	void								Load									(LPCSTR section, float condition = 1.f);

private:
	static float						s_resist_factor;
	static float						s_resist_power;

public:
	static void							loadStaticData							();
	static float						calcResist								(float d, float h);
	static float						calcPenetrationShapeFactor				(float d, float h);

	shared_str							m_ammoSect								= 0;
	float								m_fCondition							= 1.f;
	u16									bullet_material_idx						= u16_max;

	SCartridgeParam						param_s;
	Flags8								m_flags;

	shared_str							shell_particles;
	shared_str							flame_particles;
	shared_str							smoke_particles;
	shared_str							shot_particles;

	shared_str							flame_particles_flash_hider;
	shared_str							smoke_particles_silencer;
	
	bool								light_enabled;
	Fcolor								light_base_color;
	float								light_base_range;
	float								light_var_color;
	float								light_var_range;
	float								light_lifetime;

	float								weight;
};

class CWeaponAmmo :	public CInventoryItemObject
{
private:
	typedef CInventoryItemObject inherited;

public:
	virtual CWeaponAmmo		*cast_weapon_ammo		()	{return this;}
	virtual void			Load					(LPCSTR section);
	virtual BOOL			net_Spawn				(CSE_Abstract* DC);
	virtual void			net_Export				(NET_Packet& P);
	virtual void			net_Import				(NET_Packet& P);

			void			SetAmmoCount			(u16 val);
			void			ChangeAmmoCount			(int val);

			bool			Get						(CCartridge &cartridge, bool expend = true);
	xoptional<CCartridge>	getCartridge			(bool expend = true);

public:
	virtual CInventoryItem*	can_make_killing		(const CInventory *inventory) const;

	DECLARE_SCRIPT_REGISTER_FUNCTION

protected:
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);
	float								sSumItemData						O$	(EItemDataTypes type);
	xoptional<CUICellItem*>				sCreateIcon							O$	();

private:
	bool								m_can_heap								= false;
	u16									m_boxCurr								= 0;
	u16									m_box_prev								= 0;
	shared_str							m_shell_section							= 0;

	CCartridge							m_cartridge;

public:
	u16									m_boxSize								= 0;
	u8									m_mag_pos								= u8_max;

public:
	static u16							readBoxSize								(LPCSTR section);

	u16									GetAmmoCount						C$	()		{ return m_boxCurr; }
	LPCSTR								getShellSection						C$	()		{ return m_shell_section.c_str(); }

	bool								Useful								CO$ ()		{ return !!m_boxCurr; }
	Frect								GetIconRect							CO$	();
};

add_to_type_list(CWeaponAmmo)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAmmo)
