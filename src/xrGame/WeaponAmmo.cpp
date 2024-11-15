#include "stdafx.h"
#include "weaponammo.h"
#include "../xrphysics/PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "Actor_Flags.h"
#include "inventory.h"
#include "weapon.h"
#include "level_bullet_manager.h"
#include "ai_space.h"
#include "../xrEngine/gamemtllib.h"
#include "level.h"
#include "string_table.h"
#include "WeaponMagazined.h"
#include "ui\UICellCustomItems.h"

CCartridge::CCartridge() 
{
	m_flags.assign						(cfTracer | cfRicochet);
}

CCartridge::CCartridge(LPCSTR section) : CCartridge()
{
	Load								(section);
}

float CCartridge::s_resist_factor;
float CCartridge::s_resist_power;

void CCartridge::loadStaticData()
{
	s_resist_factor						= pSettings->r_float("bullet_manager", "resist_factor");
	s_resist_power						= pSettings->r_float("bullet_manager", "resist_power");
}

void CCartridge::Load(LPCSTR section, float condition)
{
	m_ammoSect							= section;
	m_fCondition						= condition;
	bullet_material_idx					= GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
	VERIFY								(bullet_material_idx != u16_max);

	param_s.kDisp						= pSettings->r_float(section, "k_disp");
	param_s.fBulletMass					= pSettings->r_float(section, "bullet_mass") * 0.001f;
	param_s.bullet_hollow_point			= !!pSettings->r_BOOL(section, "hollow_point");
	param_s.u8ColorID					= pSettings->r_u8(section, "tracer_color_ID");
	float bullet_speed					= pSettings->r_float(section, "bullet_speed") * pSettings->r_float(section, "k_bullet_speed");
	param_s.barrel_length				= pSettings->r_float(section, "reference_barrel_length");
	param_s.barrel_len					= pow(param_s.barrel_length, CWeaponMagazined::s_barrel_length_power);
	param_s.bullet_speed_per_barrel_len = bullet_speed / param_s.barrel_len;
	param_s.bullet_k_ap					= pSettings->r_float(section, "bullet_k_ap");
	
	float d								= pSettings->r_float(section, "caliber");
	float h								= pSettings->r_float(section, "tip_height");
	param_s.fBulletResist				= s_resist_factor * _sqr(d) * pow(d / h, s_resist_power);
	param_s.penetration					= param_s.fBulletMass * param_s.bullet_k_ap;
	param_s.penetration					*= d * d / (1.91f - .35f * h / d);

	param_s.fAirResist					= Level().BulletManager().m_fBulletAirResistanceScale * param_s.fBulletResist * .000001f / param_s.fBulletMass;
	param_s.fAirResistZeroingCorrection	= pow(Level().BulletManager().m_fZeroingAirResistCorrectionK1 * param_s.fAirResist, Level().BulletManager().m_fZeroingAirResistCorrectionK2);
	param_s.buckShot					= pSettings->r_s32(  section, "buck_shot");
	param_s.impair						= pSettings->r_float(section, "impair");

	m_flags.set							(cfTracer, pSettings->r_BOOL(section, "tracer"));
	m_flags.set							(cfRicochet, pSettings->r_BOOL(section, "allow_ricochet"));
	m_flags.set							(cfExplosive, pSettings->r_BOOL(section, "explosive"));
	m_flags.set							(cfMagneticBeam, pSettings->r_BOOL(section, "magnetic_beam_shot"));

	shell_particles						= pSettings->r_string(section, "shell_particles");
	flame_particles						= pSettings->r_string(section, "flame_particles");
	smoke_particles						= pSettings->r_string(section, "smoke_particles");
	shot_particles						= pSettings->r_string(section, "shot_particles");
	
	flame_particles_flash_hider			= pSettings->r_string(section, "flame_particles_flash_hider");
	smoke_particles_silencer			= pSettings->r_string(section, "smoke_particles_silencer");
	
	if (light_enabled = !pSettings->r_BOOL(section, "light_disabled"))
	{
		Fvector clr						= pSettings->r_fvector3(section, "light_color");
		light_base_color.set			(clr.x, clr.y, clr.z, 1);
		light_base_range				= pSettings->r_float(section, "light_range");
		light_var_color					= pSettings->r_float(section, "light_var_color");
		light_var_range					= pSettings->r_float(section, "light_var_range");
		light_lifetime					= pSettings->r_float(section, "light_time");
	}

	weight								= (m_ammoSect.size()) ? pSettings->r_float(m_ammoSect, "inv_weight") : 0.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CWeaponAmmo::Load(LPCSTR section) 
{
	inherited::Load						(section);
	
	m_cartridge.Load					(section);
	m_boxSize							= readBoxSize(section);
	m_can_heap							= (m_boxSize > 1);
	SetAmmoCount						(m_boxSize);
	pSettings->w_string_ex				(m_shell_section, section, "shell_section");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CWeaponAmmo::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	inherited::sSyncData				(se_obj, save);
	auto se_ammo						= se_obj->cast_item_ammo();
	if (save)
		se_ammo->m_mag_pos				= m_mag_pos;
	else
		m_mag_pos						= se_ammo->m_mag_pos;
}

float CWeaponAmmo::sSumItemData(EItemDataTypes type)
{
	return								__super::sSumItemData(type) * static_cast<float>(m_boxCurr);
}

xoptional<CUICellItem*> CWeaponAmmo::sCreateIcon()
{
	return								xr_new<CUIAmmoCellItem>(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CWeaponAmmo::net_Spawn(CSE_Abstract* DC) 
{
	BOOL bResult						= inherited::net_Spawn(DC);

	CSE_ALifeItemAmmo* l_pW				= smart_cast<CSE_ALifeItemAmmo*>(DC);
	
	if (l_pW->a_elapsed > m_boxSize)
		l_pW->a_elapsed					= m_boxSize;
	SetAmmoCount						(l_pW->a_elapsed);

	return								bResult;
}

bool CWeaponAmmo::Get(CCartridge &cartridge, bool expend)
{
	if (!m_boxCurr)
		return							false;

	cartridge							= m_cartridge;
	cartridge.m_fCondition				= GetCondition();

	if (expend)
		ChangeAmmoCount					(-1);
	if (m_pInventory)
		m_pInventory->InvalidateState	();

	return								true;
}

void CWeaponAmmo::net_Export(NET_Packet& P) 
{
	inherited::net_Export				(P);
	P.w_u16								(m_boxCurr);
}

void CWeaponAmmo::net_Import(NET_Packet& P) 
{
	inherited::net_Import				(P);
	SetAmmoCount						(P.r_u16());
}

CInventoryItem* CWeaponAmmo::can_make_killing(const CInventory *inventory) const
{
	R_ASSERT							(inventory);

	for (auto item : inventory->m_all)
	{
		CWeapon* weapon					= item->O.scast<CWeapon*>();
		if (!weapon)
			continue;

		auto i							= std::find(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end(), cNameSect());
		if (i != weapon->m_ammoTypes.end())
			return						(weapon);
	}

	return								(nullptr);
}

void CWeaponAmmo::SetAmmoCount(u16 val)
{
	m_boxCurr							= val;
	if (Useful())
	{
		if (m_box_prev < 3 || m_boxCurr < 3)
			invalidateIcon				();
		m_box_prev						= m_boxCurr;
	}
	else
		DestroyObject					();
}

void CWeaponAmmo::ChangeAmmoCount(int val)
{
	SetAmmoCount						(u16((int)m_boxCurr + val));
}

u16 CWeaponAmmo::readBoxSize(LPCSTR section)
{
	float heap_volume					= pSettings->r_float(section, "heap_volume");
	int size							= max(iFloor(heap_volume / pSettings->r_float(section, "inv_volume")), 1);
	return								static_cast<u16>(size);
}

Frect CWeaponAmmo::GetIconRect() const
{
	Frect res							= inherited::GetIconRect();
	if (m_can_heap)
	{
		if (m_boxCurr == 2)
			res.right					-= res.width() / 3.f;
		else if (m_boxCurr == 1)
			res.right					-= res.width() * 2.f / 3.f;
	}
	return								res;
}
