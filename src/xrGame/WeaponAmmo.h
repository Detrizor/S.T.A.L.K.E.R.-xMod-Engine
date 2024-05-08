#pragma once
#include "inventory_item_object.h"
#include "anticheat_dumpable_object.h"
#include "script_export_space.h"

struct SCartridgeParam
{
	float								kDisp									= 1.f;
	float								kBulletSpeed							= 1.f;
	float								impair									= 1.f;
	float								fBulletMass								= 1.f;
	float								fBulletResist							= 1.f;
	float								fWMS									= -1.f;
	float								fAirResist								= 1.f;
	float								fAirResistZeroingCorrection				= 1000.f;
	float								bullet_k_ap								= 1.f;
	float								muzzle_velocity							= 1.f;
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
	shared_str							m_ammoSect								= 0;
	float								m_fCondition							= 1.f;
	u16									bullet_material_idx						= u16_max;

	SCartridgeParam						param_s;
	Flags8								m_flags;

	float								Weight								C$	();

	void							S$	loadStaticVariables						();
	float							S$	calcResist								(LPCSTR section);
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

public:
	virtual CInventoryItem*	can_make_killing		(const CInventory *inventory) const;

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	CCartridge							m_cartridge;
	bool								m_can_heap;
	
	u16									m_boxCurr								= 0;

protected:
	float								Aboba								O$	(EEventTypes type, void* data = NULL, int param = 0);

public:
	u16									m_boxSize								= 0;
	
	float							S$	readBoxSize								(LPCSTR section);
	
	u16									GetAmmoCount						C$	()		{ return m_boxCurr; }
	bool								Useful								CO$ ()		{ return !!m_boxCurr; }
	Frect								GetIconRect							CO$	();
};

add_to_type_list(CWeaponAmmo)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAmmo)
