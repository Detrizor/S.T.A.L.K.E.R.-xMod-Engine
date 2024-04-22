#pragma once
#include "inventory_item_object.h"
#include "anticheat_dumpable_object.h"
#include "script_export_space.h"

struct SCartridgeParam
{
	float	kDisp, kBulletSpeed, impair, fBulletMass, fBulletResist, fWMS, fAirResist, fAirResistZeroingCorrection;
	int		buckShot;
	u8		u8ColorID;
	bool	mHollowPoint;
	bool	mArmorPiercing;

	IC void Init()
	{
		mHollowPoint = mArmorPiercing = false;
		kDisp = kBulletSpeed = impair = 1.0f;
		fBulletMass = fBulletResist = 0.0f;
		fWMS		= -1.f;
		fAirResist	= 1.f;
		fAirResistZeroingCorrection = 1000.f;
		buckShot	= 1;
		u8ColorID	= 0;
	}
};

class CCartridge : public IAnticheatDumpable
{
public:
	CCartridge();
	void Load(LPCSTR section, float condition = 1.f);
	float Weight() const;
	float Volume() const;

	shared_str	m_ammoSect;
	enum{
		cfTracer				= (1<<0),
		cfRicochet				= (1<<1),
		cfCanBeUnlimited		= (1<<2),
		cfExplosive				= (1<<3),
		cfMagneticBeam			= (1<<4),
	};
	SCartridgeParam param_s;

	u16		bullet_material_idx;
	Flags8	m_flags;

	float	m_fCondition;

	virtual shared_str const 	GetAnticheatSectionName	() const { return m_ammoSect; };
};

class CWeaponAmmo :	public CInventoryItemObject
{
private:
	typedef CInventoryItemObject		inherited;

public:
							CWeaponAmmo				(void);
	virtual					~CWeaponAmmo			(void);

private:
			u16				m_boxCurr;

public:
	virtual CWeaponAmmo		*cast_weapon_ammo		()	{return this;}
	virtual void			Load					(LPCSTR section);
	virtual BOOL			net_Spawn				(CSE_Abstract* DC);
	virtual void			net_Destroy				();
	virtual void			net_Export				(NET_Packet& P);
	virtual void			net_Import				(NET_Packet& P);
	virtual void			OnH_B_Chield			();
	virtual void			OnH_B_Independent		(bool just_before_destroy);
	virtual void			UpdateCL				();
	virtual void			renderable_Render		();
	virtual bool			Useful					() const;

			u16				GetAmmoCount			() const;
			void			SetAmmoCount			(u16 val);
			void			ChangeAmmoCount			(int val);

			bool			Get						(CCartridge &cartridge, bool expend = true);

			SCartridgeParam	cartridge_param;

			u16				m_boxSize;
			bool			m_tracer;
			bool			m_4to1_tracer;

public:
	virtual CInventoryItem*	can_make_killing		(const CInventory *inventory) const;

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	bool								m_bHeap;

protected:
	float								Aboba								O$	(EEventTypes type, void* data = NULL, int param = 0);

public:
	Frect								GetIconRect							CO$	();
};

add_to_type_list(CWeaponAmmo)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAmmo)
