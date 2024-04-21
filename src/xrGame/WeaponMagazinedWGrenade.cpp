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
	m_ammoType2 = 0;
	m_bGrenadeMode = false;
	iMagazineSize2 = 0;
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
	m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);

	// load ammo classes SECOND (grenade_class)
	m_ammoTypes2.clear();
	LPCSTR				S = pSettings->r_string(section, "grenade_class");
	if (S && S[0])
	{
		string128		_ammoItem;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, _ammoItem);
			m_ammoTypes2.push_back(_ammoItem);
		}
	}

	iMagazineSize2 = iMagazineSize;

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

	UpdateGrenadeVisibility(!!iAmmoElapsed);
	SetPending(FALSE);
	
	m_ammoType2						= weapon->a_elapsed_grenades.grenades_type;
	m_DefaultCartridge2.Load		(*m_ammoTypes2[m_ammoType2], m_ammoType2);

	xr_vector<CCartridge>* pM = NULL;
	bool b_if_grenade_mode = (m_bGrenadeMode && iAmmoElapsed && !getRocketCount());
	if (b_if_grenade_mode)
		pM = &m_magazine;

	bool b_if_simple_mode = (!m_bGrenadeMode && m_magazine2.size() && !getRocketCount());
	if (b_if_simple_mode)
		pM = &m_magazine2;

	if (b_if_grenade_mode || b_if_simple_mode)
	{
		shared_str fake_grenade_name = pSettings->r_string(pM->back().m_ammoSect, "fake_grenade_name");

		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}

	return l_res;
}

u8 CWeaponMagazinedWGrenade::GetGrenade()
{
	return u8((m_bGrenadeMode) ? m_magazine.size() : m_magazine2.size());
}

void CWeaponMagazinedWGrenade::SetGrenade(u8 cnt)
{
	xr_vector<CCartridge>& magazine		= (m_bGrenadeMode) ? m_magazine : m_magazine2;
	CCartridge							cartridge;
	if (m_bGrenadeMode)
		cartridge.Load					(*m_ammoTypes[m_ammoType], m_ammoType);
	else
		cartridge.Load					(*m_ammoTypes2[m_ammoType2], m_ammoType2);
	while (magazine.size() < cnt)
		magazine.push_back				(cartridge);
}

void CWeaponMagazinedWGrenade::switch2_Reload()
{
	VERIFY(GetState() == eReload);
	if (m_bGrenadeMode)
	{
		PlaySound("sndReloadG", m_muzzle_position_gl);
		PlayHUDMotion("anm_reload", FALSE, this, GetState());
		SetPending(TRUE);
	}
	else
		inherited::switch2_Reload();
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

	bool bUsefulStateToSwitch = ((eIdle == GetState()) || (eHidden == GetState()) || (eMisfire == GetState()) || (eMagEmpty == GetState())) && (!IsPending());

	if (!bUsefulStateToSwitch)
		return false;

	OnZoomOut();

	SetPending(TRUE);

	PerformSwitchGL();

	PlaySound("sndSwitch", get_LastFP());

	m_BriefInfo_CalcFrame = 0;

	return PlayAnimModeSwitch();
}

void  CWeaponMagazinedWGrenade::PerformSwitchGL()
{
	m_bGrenadeMode = !m_bGrenadeMode;
	m_MotionsSuffix = (m_bGrenadeMode) ? "g" : ((m_pLauncher) ? m_pLauncher->cast<CAddon*>()->MotionSuffix() : 0);

	iMagazineSize = m_bGrenadeMode ? 1 : iMagazineSize2;

	m_ammoTypes.swap(m_ammoTypes2);

	swap(m_ammoType, m_ammoType2);
	swap(m_DefaultCartridge, m_DefaultCartridge2);

	m_magazine.swap(m_magazine2);
	iAmmoElapsed = m_magazine.size();
	if (!m_bGrenadeMode && m_pMagazine)
		iAmmoElapsed += m_pMagazine->Amount();

	m_BriefInfo_CalcFrame = 0;

	m_hud->SwitchGL();
}

xr_vector<CCartridge>& CWeaponMagazinedWGrenade::Magazine()
{
	return (m_bGrenadeMode) ? m_magazine2 : m_magazine;
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags)
{
	if (m_bGrenadeMode && cmd == kWPN_FIRE)
	{
		if (IsPending())
			return				false;

		if (flags&CMD_START)
		{
			if (iAmmoElapsed)
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
	VERIFY(fOneShotTime > 0.f);

	//режим стрельбы подствольника
	if (m_bGrenadeMode)
	{
		/*
		fTime					-=dt;
		while (fTime<=0 && (iAmmoElapsed>0) && (IsWorking() || m_bFireSingleShot))
		{
		++m_iShotNum;
		OnShot			();

		// Ammo
		if(Local())
		{
		VERIFY				(m_magazine.size());
		m_magazine.pop_back	();
		--iAmmoElapsed;

		VERIFY((u32)iAmmoElapsed == m_magazine.size());
		}
		}
		UpdateSounds				();
		if(m_iShotNum == m_iQueueSize)
		FireEnd();
		*/
	}
	//режим стрельбы очередями
	else
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

void  CWeaponMagazinedWGrenade::LaunchGrenade()
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
			{
				d = res[0];
			};
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
		VERIFY(m_magazine.size());
		m_magazine.pop_back();
		--iAmmoElapsed;
		VERIFY((u32) iAmmoElapsed == m_magazine.size());

		NET_Packet					P;
		u_EventGen(P, GE_LAUNCH_ROCKET, ID());
		P.w_u16(getCurrentRocket()->ID());
		u_EventSend(P);
	};
}

void CWeaponMagazinedWGrenade::FireEnd()
{
	if (m_bGrenadeMode)
	{
		CWeapon::FireEnd();
	}
	else
		inherited::FireEnd();
}

void CWeaponMagazinedWGrenade::OnMagazineEmpty()
{
	if (GetState() == eIdle)
	{
		OnEmptyClick();
	}
}

void CWeaponMagazinedWGrenade::ReloadMagazine()
{
	inherited::ReloadMagazine();

	//перезарядка подствольного гранатомета
	if (iAmmoElapsed && !getRocketCount() && m_bGrenadeMode)
	{
		shared_str fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name");

		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S, u32 oldState)
{
	switch (S)
	{
	case eSwitch:
	{
		if (!SwitchMode())
		{
			SwitchState(eIdle);
			return;
		}
	}break;
	}

	inherited::OnStateSwitch(S, oldState);
	UpdateGrenadeVisibility(!!iAmmoElapsed || S == eReload);
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
		PlayHUDMotion("anm_switch", TRUE, this, eSwitch);
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

void CWeaponMagazinedWGrenade::UpdateGrenadeVisibility(bool visibility)
{
	if (!GetHUDmode())							return;
	HudItemData()->set_bone_visible("grenade", visibility, TRUE);
}

void CWeaponMagazinedWGrenade::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(m_bGrenadeMode, output_packet);
	save_data(m_magazine2.size(), output_packet);
}

void CWeaponMagazinedWGrenade::load(IReader &input_packet)
{
	inherited::load(input_packet);
	bool b;
	load_data(b, input_packet);
	if (b != m_bGrenadeMode)
		SwitchMode();

	u32 sz;
	load_data(sz, input_packet);

	CCartridge					l_cartridge;
	l_cartridge.Load(m_ammoTypes2[m_ammoType2].c_str(), m_ammoType2);

	while (sz > m_magazine2.size())
		m_magazine2.push_back(l_cartridge);
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
		std::find(m_ammoTypes2.begin(), m_ammoTypes2.end(), item_sect) != m_ammoTypes2.end()
		);
}

bool CWeaponMagazinedWGrenade::install_upgrade_ammo_class(LPCSTR section, bool test)
{
	LPCSTR str;

	bool result = process_if_exists(section, "ammo_mag_size", iMagazineSize2, test);
	iMagazineSize = m_bGrenadeMode ? 1 : iMagazineSize2;

	bool result2 = process_if_exists(section, "ammo_class", str, test);
	if (result2 && !test)
	{
		xr_vector<shared_str>& ammo_types = m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
		ammo_types.clear();
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128						ammo_item;
			_GetItem(str, i, ammo_item);
			ammo_types.push_back(ammo_item);
		}

		m_ammoType = 0;
		m_ammoType2 = 0;
	}
	result |= result2;

	return result2;
}

bool CWeaponMagazinedWGrenade::install_upgrade_impl(LPCSTR section, bool test)
{
	LPCSTR str;
	bool result = inherited::install_upgrade_impl(section, test);

	bool result2 = process_if_exists(section, "grenade_class", str, test);
	if (result2 && !test)
	{
		xr_vector<shared_str>& ammo_types = !m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
		ammo_types.clear();
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128						ammo_item;
			_GetItem(str, i, ammo_item);
			ammo_types.push_back(ammo_item);
		}

		m_ammoType = 0;
		m_ammoType2 = 0;
	}
	result |= result2;

	result2 = process_if_exists(section, "snd_shoot_grenade", str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", false, m_eSoundShot);
	}
	result |= result2;

	result2 = process_if_exists(section, "snd_reload_grenade", str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	}
	result |= result2;

	result2 = process_if_exists(section, "snd_switch", str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);
	}
	result |= result2;

	return result;
}

void CWeaponMagazinedWGrenade::net_Spawn_install_upgrades(Upgrades_type saved_upgrades)
{
	// do not delete this
	// this is intended behaviour
}

int CWeaponMagazinedWGrenade::GetAmmoCount2(u8 ammo2_type) const
{
	VERIFY(m_pInventory);
	R_ASSERT(ammo2_type < m_ammoTypes2.size());

	return GetAmmoCount_forType(m_ammoTypes2[ammo2_type]);
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
		if (!m_bGrenadeMode)
			PerformSwitchGL				();
		if (m_pInventory)
			inventory_owner().Discharge	(this, true);
		PerformSwitchGL					();
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
			return						inherited::Aboba(type, data, param) + GetMagazineWeight(m_magazine2);
	}

	return								inherited::Aboba(type, data, param);
}

int CWeaponMagazinedWGrenade::Chamber C$()
{
	return (m_bGrenadeMode) ? 0 : inherited::Chamber();
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
