#include "stdafx.h"
#include "weaponmagazinedwgrenade.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "GrenadeLauncher.h"
#include "xrserver_objects_alife_items.h"
#include "ExplosiveRocket.h"
#include "Actor.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "game_base_space.h"
#include "../xrphysics/MathUtils.h"
#include "player_hud.h"
#include "Magazine.h"
#include "../build_config_defines.h"
#include "weapon_hud.h"

#ifdef DEBUG
#	include "phdebug.h"
#endif

#include "addon.h"

CWeaponMagazinedWGrenade::CWeaponMagazinedWGrenade(ESoundTypes eSoundType) : CWeaponMagazined(eSoundType)
{
	m_fLaunchSpeed = 0.f;
}

CWeaponMagazinedWGrenade::~CWeaponMagazinedWGrenade()
{}

void CWeaponMagazinedWGrenade::Load(LPCSTR section)
{
	inherited::Load(section);
	CRocketLauncher::Load(section);

	//// Sounds
	m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", true, m_eSoundShot);
	m_sounds.LoadSound(*HudSection(), "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	m_sounds.LoadSound(*HudSection(), "snd_switch", "sndSwitch", true, m_eSoundReload);

	// load ammo classes SECOND (grenade_class)
	LPCSTR				S = pSettings->r_string(section, "grenade_class");
	if (S && S[0])
	{
		string128		_ammoItem;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, _ammoItem);
			m_grenade_types.push_back(_ammoItem);
		}
	}

	shared_str integrated_gl			= READ_IF_EXISTS(pSettings, r_string, section, "grenade_launcher", "");
	if (integrated_gl.size())
		ProcessGL						(xr_new<CGrenadeLauncher>(this, integrated_gl), true);

	if (pSettings->line_exist(section, "flame_particles_gl"))
		m_flame_particles_gl_name = pSettings->r_string(section, "flame_particles_gl");
	else if (pSettings->line_exist(section, "flame_particles"))
		m_flame_particles_gl_name = pSettings->r_string(section, "flame_particles");
}

void CWeaponMagazinedWGrenade::net_Destroy()
{
	inherited::net_Destroy();
	stop_flame_particles_gl();
}

BOOL CWeaponMagazinedWGrenade::net_Spawn(CSE_Abstract* DC)
{
	CSE_ALifeItemWeapon* const weapon = smart_cast<CSE_ALifeItemWeapon*>(DC);
	R_ASSERT(weapon);
	inherited::net_Spawn_install_upgrades(weapon->m_upgrades);

	BOOL l_res = inherited::net_Spawn(DC);

	SetPending(FALSE);
	
	m_grenade_type = weapon->a_elapsed_grenades.grenades_type;

	if (m_grenade && !getRocketCount())
	{
		shared_str fake_grenade_name = pSettings->r_string(m_grenade->m_ammoSect, "fake_grenade_name");
		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}

	return l_res;
}

u8 CWeaponMagazinedWGrenade::GetGrenade() const
{
	return u8(!!m_grenade);
}

bool CWeaponMagazinedWGrenade::canTake(CWeaponAmmo CPC ammo, bool chamber) const
{
	if (!m_bGrenadeMode || chamber)
		return							inherited::canTake(ammo, chamber);
	
	if (!m_grenade)
		for (auto& t : m_grenade_types)
			if (t == ammo->Section())
				return					true;
	return								false;
}

void CWeaponMagazinedWGrenade::SetGrenade(u8 cnt)
{
	m_grenade->Load						(*m_grenade_types[m_grenade_type]);
}

void CWeaponMagazinedWGrenade::PlayAnimReload()
{
	if (m_bGrenadeMode)
	{
		PlaySound("sndReloadG", m_muzzle_position_gl);
		PlayHUDMotion("anm_reload", FALSE, GetState());
	}
	else
		inherited::PlayAnimReload();
}

void CWeaponMagazinedWGrenade::OnShot()
{
	if (m_bGrenadeMode)
		shoot_grenade();
	else
		inherited::OnShot();
}

bool CWeaponMagazinedWGrenade::SwitchMode()
{
	if (!m_pLauncher)
		return false;

	bool bUsefulStateToSwitch = ((eIdle == GetState()) || (eHidden == GetState()) || (eMisfire == GetState())) && (!IsPending());
	if (!bUsefulStateToSwitch)
		return false;

	OnZoomOut();
	SetPending(TRUE);
	PerformSwitchGL();
	PlaySound("sndSwitch", get_LastFP());
	m_BriefInfo_CalcFrame = 0;

	return PlayAnimModeSwitch();
}

void CWeaponMagazinedWGrenade::PerformSwitchGL()
{
	m_bGrenadeMode = !m_bGrenadeMode;
	m_MotionsSuffix = (m_bGrenadeMode) ? "g" : ((m_pLauncher) ? m_pLauncher->cast<CAddon*>()->MotionSuffix() : 0);

	m_hud->SwitchGL();
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags)
{
	if (m_bGrenadeMode && cmd == kWPN_FIRE)
	{
		if (IsPending())
			return				false;

		if (flags&CMD_START)
		{
			if (m_grenade)
				LaunchGrenade();

			if (GetState() == eIdle)
				OnEmptyClick();
		}
		return					true;
	}
	if (inherited::Action(cmd, flags))
		return true;

	switch (cmd)
	{
	case kWPN_FUNC:
	{
		if (flags&CMD_START && !IsPending())
			SwitchState(eSwitch);
		return true;
	}
	}
	return false;
}

#include "inventory.h"
#include "inventoryOwner.h"
void CWeaponMagazinedWGrenade::state_Fire(float dt)
{
	if (!m_bGrenadeMode)
		inherited::state_Fire(dt);
}

void CWeaponMagazinedWGrenade::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent					(P, type);
	if (type == GE_LAUNCH_ROCKET)
	{
		u16								id;
		P.r_u16							(id);
		CRocketLauncher::DetachRocket	(id, true);
		shoot_grenade                   ();
	}
}

void CWeaponMagazinedWGrenade::LaunchGrenade()
{
	if (!getRocketCount())	return;
	R_ASSERT(m_bGrenadeMode);

	Fvector						p1, d;
	p1.set(m_muzzle_position_gl);
	d.set(get_LastFD());
	CEntity*					E = smart_cast<CEntity*>(H_Parent());

	if (E)
	{
		CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
		if (NULL == io->inventory().ActiveItem())
		{
			Log("current_state", GetState());
			Log("next_state", GetNextState());
			Log("item_sect", cNameSect().c_str());
			Log("H_Parent", H_Parent()->cNameSect().c_str());
		}
		E->g_fireParams(this, p1, d);
	}
	p1.set(m_muzzle_position_gl);

	Fmatrix							launch_matrix;
	launch_matrix.identity();
	launch_matrix.k.set(d);
	Fvector::generate_orthonormal_basis(launch_matrix.k,
		launch_matrix.j,
		launch_matrix.i);

	launch_matrix.c.set(p1);

	if (ADS())
	{
		H_Parent()->setEnabled(FALSE);
		setEnabled(FALSE);

		collide::rq_result			RQ;
		BOOL HasPick = Level().ObjectSpace.RayPick(p1, d, 300.0f, collide::rqtStatic, RQ, this);

		setEnabled(TRUE);
		H_Parent()->setEnabled(TRUE);

		if (HasPick)
		{
			Fvector					Transference;
			Transference.mul(d, RQ.range);
			Fvector					res[2];
			u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference,
				CRocketLauncher::m_fLaunchSpeed,
				EffectiveGravity(),
				res);

			if (canfire0 != 0)
				d = res[0];
		}
	};

	d.normalize();
	d.mul(CRocketLauncher::m_fLaunchSpeed);
	VERIFY2(_valid(launch_matrix), "CWeaponMagazinedWGrenade::SwitchState. Invalid launch_matrix!");
	CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

	CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
	VERIFY(pGrenade);
	pGrenade->SetInitiator(H_Parent()->ID());

	if (Local() && OnServer())
	{
		m_grenade = NULL;

		NET_Packet					P;
		u_EventGen(P, GE_LAUNCH_ROCKET, ID());
		P.w_u16(getCurrentRocket()->ID());
		u_EventSend(P);
	};
}

void CWeaponMagazinedWGrenade::OnMagazineEmpty()
{
	if (GetState() == eIdle)
		OnEmptyClick();
}

void CWeaponMagazinedWGrenade::ReloadMagazine()
{
	if (m_bGrenadeMode)
	{
		m_grenade->Load(m_grenade_types[m_grenade_type].c_str());
		if (!getRocketCount())
		{
			shared_str fake_grenade_name = pSettings->r_string(m_grenade->m_ammoSect, "fake_grenade_name");
			CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
		}
	}
	else
		inherited::ReloadMagazine		();
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S, u32 oldState)
{
	switch (S)
	{
	case eSwitch:
		if (!SwitchMode())
		{
			SwitchState(eIdle);
			return;
		}
		break;
	}

	inherited::OnStateSwitch(S, oldState);
}

void CWeaponMagazinedWGrenade::OnAnimationEnd(u32 state)
{
	switch (state)
	{
		case eSwitch:
			SwitchState(eIdle);
			break;
	}
	inherited::OnAnimationEnd(state);
}

void CWeaponMagazinedWGrenade::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	SetPending(FALSE);
	if (m_bGrenadeMode)
	{
		SetState(eIdle);
		SetPending(FALSE);
	}
}

bool CWeaponMagazinedWGrenade::PlayAnimModeSwitch()
{
	if (HudAnimationExist("anm_switch"))
	{
		PlayHUDMotion("anm_switch", TRUE, eSwitch);
		return true;
	}
	return false;
}

void CWeaponMagazinedWGrenade::UpdateSounds()
{
	inherited::UpdateSounds();
	Fvector P = get_LastFP();
	m_sounds.SetPosition("sndShotG", P);
	m_sounds.SetPosition("sndReloadG", P);
	m_sounds.SetPosition("sndSwitch", P);
}

void CWeaponMagazinedWGrenade::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(m_bGrenadeMode, output_packet);
	save_data(!!m_grenade, output_packet);
}

void CWeaponMagazinedWGrenade::load(IReader &input_packet)
{
	inherited::load(input_packet);
	bool b;
	load_data(b, input_packet);
	if (b != m_bGrenadeMode)
		SwitchMode();

	load_data(b, input_packet);
	if (b)
		m_grenade->Load(m_grenade_types[m_grenade_type].c_str());
}

void CWeaponMagazinedWGrenade::net_Export(NET_Packet& P)
{
	P.w_u8(m_bGrenadeMode ? 1 : 0);

	inherited::net_Export(P);
}

void CWeaponMagazinedWGrenade::net_Import(NET_Packet& P)
{
	bool NewMode = FALSE;
	NewMode = !!P.r_u8();
	if (NewMode != m_bGrenadeMode)
		SwitchMode();

	inherited::net_Import(P);
}

bool CWeaponMagazinedWGrenade::IsNecessaryItem(const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() ||
		std::find(m_grenade_types.begin(), m_grenade_types.end(), item_sect) != m_grenade_types.end());
}

bool CWeaponMagazinedWGrenade::install_upgrade_impl(LPCSTR section, bool test)
{
	LPCSTR str;
	bool result = inherited::install_upgrade_impl(section, test);

	bool result2 = process_if_exists(section, "grenade_class", str, test);
	if (result2 && !test)
	{
		m_grenade_types.clear();
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128						ammo_item;
			_GetItem(str, i, ammo_item);
			m_grenade_types.push_back(ammo_item);
		}
		m_grenade_type = 0;
	}
	result |= result2;

	result2 = process_if_exists(section, "snd_shoot_grenade", str, test);
	if (result2 && !test)
		m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", false, m_eSoundShot);
	result |= result2;

	result2 = process_if_exists(section, "snd_reload_grenade", str, test);
	if (result2 && !test)
		m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	result |= result2;

	result2 = process_if_exists(section, "snd_switch", str, test);
	if (result2 && !test)
		m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);
	result |= result2;

	return result;
}

void CWeaponMagazinedWGrenade::net_Spawn_install_upgrades(Upgrades_type saved_upgrades)
{
	// do not delete this
	// this is intended behaviour
}

void CWeaponMagazinedWGrenade::ProcessGL(CGrenadeLauncher* gl, bool attach)
{
	m_pLauncher							= (attach) ? gl : NULL;

	if (attach)
	{
		m_hud->ProcessGL				(gl);
		m_fLaunchSpeed					= gl->GetGrenadeVel();
		m_flame_particles_gl_name		= gl->FlameParticles();
	}
	else
	{
		if (m_pInventory)
			inventory_owner().GiveAmmo	(*m_grenade->m_ammoSect, 1, m_grenade->m_fCondition);
		if (m_bGrenadeMode)
			PerformSwitchGL				();
	}
}

void CWeaponMagazinedWGrenade::shoot_grenade()
{
	PlayAnimShoot();
	PlaySound("sndShotG", m_muzzle_position_gl);
	AddShotEffector();
	start_flame_particles_gl();
}

float CWeaponMagazinedWGrenade::Aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eOnChild:
		{
			CObject* obj				= (CObject*)data;
			if (cast<CCustomRocket*>(obj))
			{
				if (param)
					AttachRocket		(obj->ID(), this);
				else
					DetachRocket		(obj->ID(), false);
			}
			break;
		}

		case eWeight:
			return						inherited::Aboba(type, data, param);		//--xd need implement grenade weight when reworking underbarrel gls
	}

	return								inherited::Aboba(type, data, param);
}

bool CWeaponMagazinedWGrenade::HasAltAim C$()
{
	return (m_bGrenadeMode) ? FALSE : inherited::HasAltAim();
}

void CWeaponMagazinedWGrenade::SetADS(int mode)
{
	if (m_bGrenadeMode && mode == 1)
		mode = 2;
	inherited::SetADS(mode);
}

bool CWeaponMagazinedWGrenade::AltHandsAttach() const
{
	return m_bGrenadeMode;// && ADS();
}

void CWeaponMagazinedWGrenade::start_flame_particles_gl()
{
	CShootingObject::StartParticles(m_flame_particles_gl, *m_flame_particles_gl_name, m_muzzle_position_gl);
}
void CWeaponMagazinedWGrenade::stop_flame_particles_gl()
{
	CShootingObject::StopParticles (m_flame_particles_gl);
}
void CWeaponMagazinedWGrenade::update_flame_particles_gl()
{
	if (m_flame_particles_gl)
		CShootingObject::UpdateParticles(m_flame_particles_gl, m_muzzle_position_gl);
}

void CWeaponMagazinedWGrenade::UpdateCL()
{
	inherited::UpdateCL();
	update_flame_particles_gl();
}

void CWeaponMagazinedWGrenade::process_addon(CAddon* addon, bool attach)
{
	CGrenadeLauncher* gl				= addon->cast<CGrenadeLauncher*>();
	if (gl)
		ProcessGL						(gl, attach);
	inherited::process_addon			(addon, attach);
}
