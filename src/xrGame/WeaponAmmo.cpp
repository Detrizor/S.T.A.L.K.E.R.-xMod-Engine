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

CCartridge::CCartridge() 
{
	m_flags.assign			(cfTracer | cfRicochet);
	m_ammoSect = NULL;
	param_s.Init();
	bullet_material_idx = u16(-1);
	m_fCondition = 1.f;
}

void CCartridge::Load(LPCSTR section, float condition)
{
	m_ammoSect				= section;
	param_s.kDisp				= pSettings->r_float(section, "k_disp");
	param_s.fBulletMass			= pSettings->r_float(section, "bullet_mass") * 0.001f;
	param_s.mHollowPoint		= !!pSettings->r_bool(section, "hollow_point");
	param_s.mArmorPiercing		= !!pSettings->r_bool(section, "armor_piercing");
	param_s.u8ColorID			= READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);
	param_s.kBulletSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "k_bullet_speed", 1.f);

	float caliber			= pSettings->r_float(section, "caliber");
	float area				= PI * pow((caliber / 2.f), 2);
	float sharpness			= pSettings->r_float(section, "sharpness");
	param_s.fBulletResist	= area / sharpness;
	param_s.fAirResist		= Level().BulletManager().m_fBulletAirResistanceScale * param_s.fBulletResist * .000001f / param_s.fBulletMass;
	param_s.fAirResistZeroingCorrection = pow(Level().BulletManager().m_fZeroingAirResistCorrectionK1 * param_s.fAirResist, Level().BulletManager().m_fZeroingAirResistCorrectionK2);
	m_fCondition			= condition;

	m_flags.set					(cfTracer, pSettings->r_bool(section, "tracer"));
	param_s.buckShot			= pSettings->r_s32(  section, "buck_shot");
	param_s.impair				= pSettings->r_float(section, "impair");
	
	m_flags.set					(cfRicochet, TRUE);
	m_flags.set					(cfMagneticBeam, FALSE);

	if (pSettings->line_exist(section, "allow_ricochet"))
	{
		if (!pSettings->r_bool(section, "allow_ricochet"))
			m_flags.set(cfRicochet, FALSE);
	}
	if (pSettings->line_exist(section, "magnetic_beam_shot"))
	{
		if (pSettings->r_bool(section, "magnetic_beam_shot"))
			m_flags.set(cfMagneticBeam, TRUE);
	}

	m_flags.set			(cfExplosive, pSettings->r_bool(section, "explosive"));

	bullet_material_idx		=  GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
	VERIFY	(u16(-1)!=bullet_material_idx);
}

float CCartridge::Weight() const
{
	return (m_ammoSect.size()) ? pSettings->r_float(m_ammoSect, "inv_weight") : 0.f;
}

float CCartridge::Volume() const
{
	return (m_ammoSect.size()) ? pSettings->r_float(m_ammoSect, "inv_volume") : 0.f;
}

CWeaponAmmo::CWeaponAmmo(void) 
{
	m_4to1_tracer = false;
	m_boxSize = 0;
	m_boxCurr = 0;
	cartridge_param.Init();
}

CWeaponAmmo::~CWeaponAmmo(void)
{
}

void CWeaponAmmo::Load(LPCSTR section) 
{
	inherited::Load			(section);

	cartridge_param.kDisp			= pSettings->r_float(section, "k_disp");
	cartridge_param.fBulletMass		= pSettings->r_float(section, "bullet_mass") * 0.001f;
	cartridge_param.mHollowPoint	= !!pSettings->r_bool(section, "hollow_point");
	cartridge_param.mArmorPiercing	= !!pSettings->r_bool(section, "armor_piercing");
	cartridge_param.u8ColorID		= READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);

	float caliber					= pSettings->r_float(section, "caliber");
	float area						= PI * pow((caliber / 2.f), 2);
	float sharpness					= pSettings->r_float(section, "sharpness");
	cartridge_param.fBulletResist	= area / sharpness;
	cartridge_param.fAirResist		= Level().BulletManager().m_fBulletAirResistanceScale * cartridge_param.fBulletResist * .000001f / cartridge_param.fBulletMass;
	cartridge_param.fAirResistZeroingCorrection = pow(Level().BulletManager().m_fZeroingAirResistCorrectionK1 * cartridge_param.fAirResist, Level().BulletManager().m_fZeroingAirResistCorrectionK2);

	m_tracer						= !!pSettings->r_bool(section, "tracer");

	if (pSettings->line_exist(section, "4to1_tracer"))
		m_4to1_tracer = !!pSettings->r_bool(section, "4to1_tracer");

	cartridge_param.kBulletSpeed = READ_IF_EXISTS(pSettings, r_float, section, "k_bullet_speed", 1.0f);

	cartridge_param.buckShot		= pSettings->r_s32(  section, "buck_shot");
	cartridge_param.impair			= pSettings->r_float(section, "impair");

	m_boxSize				= (u16)floor(1.f/m_volume);
	m_boxCurr				= m_boxSize;

	m_bHeap = pSettings->r_bool(m_section_id, "heap");
}

BOOL CWeaponAmmo::net_Spawn(CSE_Abstract* DC) 
{
	BOOL bResult			= inherited::net_Spawn	(DC);
	CSE_Abstract	*e		= (CSE_Abstract*)(DC);
	CSE_ALifeItemAmmo* l_pW	= smart_cast<CSE_ALifeItemAmmo*>(e);
	m_boxCurr				= l_pW->a_elapsed;
	
	if(m_boxCurr > m_boxSize)
		l_pW->a_elapsed		= m_boxCurr = m_boxSize;

	return					bResult;
}

void CWeaponAmmo::net_Destroy() 
{
	inherited::net_Destroy	();
}

void CWeaponAmmo::OnH_B_Chield() 
{
	inherited::OnH_B_Chield	();
}

void CWeaponAmmo::OnH_B_Independent(bool just_before_destroy) 
{
	if(!Useful()) {
		
		if (Local()){
			DestroyObject	();
		}
		m_ready_to_destroy	= true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}


bool CWeaponAmmo::Useful() const
{
	return !!m_boxCurr;
}

bool CWeaponAmmo::Get(CCartridge &cartridge, bool expend)
{
	if(!m_boxCurr) return false;
	cartridge.m_ammoSect = cNameSect();
	
	cartridge.param_s = cartridge_param;

	cartridge.m_flags.set(CCartridge::cfTracer ,m_tracer);
	cartridge.bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
	cartridge.m_fCondition = GetCondition();
	if (expend)
		ChangeAmmoCount(-1);
	if(m_pInventory)m_pInventory->InvalidateState();
	return true;
}

void CWeaponAmmo::renderable_Render() 
{
	if(!m_ready_to_destroy)
		inherited::renderable_Render();
}

void CWeaponAmmo::UpdateCL() 
{
	VERIFY2								(_valid(renderable.xform),*cName());
	inherited::UpdateCL	();
	VERIFY2								(_valid(renderable.xform),*cName());
}

void CWeaponAmmo::net_Export(NET_Packet& P) 
{
	inherited::net_Export	(P);
	
	P.w_u16					(m_boxCurr);
}

void CWeaponAmmo::net_Import(NET_Packet& P) 
{
	inherited::net_Import	(P);

	P.r_u16					(m_boxCurr);
}

CInventoryItem *CWeaponAmmo::can_make_killing	(const CInventory *inventory) const
{
	VERIFY					(inventory);

	TIItemContainer::const_iterator	I = inventory->m_all.begin();
	TIItemContainer::const_iterator	E = inventory->m_all.end();
	for ( ; I != E; ++I) {
		CWeapon		*weapon = smart_cast<CWeapon*>(*I);
		if (!weapon)
			continue;
		xr_vector<shared_str>::const_iterator	i = std::find(weapon->m_ammoTypes.begin(),weapon->m_ammoTypes.end(),cNameSect());
		if (i != weapon->m_ammoTypes.end())
			return			(weapon);
	}

	return					(0);
}

u16 CWeaponAmmo::GetAmmoCount() const
{
	return m_boxCurr;
}

void CWeaponAmmo::SetAmmoCount(u16 val)
{
	m_boxCurr = val;

	if (!Useful() && CInventoryItem::object().Local() && OnServer())
		CInventoryItem::object().DestroyObject();
}

void CWeaponAmmo::ChangeAmmoCount(int val)
{
	SetAmmoCount(u16((int)m_boxCurr + val));
}

float CWeaponAmmo::Aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eWeight:
		case eVolume:
		case eCost:
			return						inherited::Aboba(type, data, param) * (float)m_boxCurr;
	}

	return								inherited::Aboba(type, data, param);
}

Frect CWeaponAmmo::GetIconRect() const
{
	Frect res = inherited::GetIconRect();
	if (m_bHeap)
	{
		if (m_boxCurr == 2)
			res.right -= res.width() / 3.f;
		else if (m_boxCurr == 1)
			res.right -= res.width() * 2.f / 3.f;
	}
	return res;
}
