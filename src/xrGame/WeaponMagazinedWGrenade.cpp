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
#include "inventory.h"
#include "inventoryOwner.h"
#include "addon.h"


void CWeaponMagazinedWGrenade::Load(LPCSTR section)
{
	inherited::Load						(section);
	CRocketLauncher::Load				(section);

	//// Sounds
	m_sounds.LoadSound					(*HudSection(), "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_switch", "sndSwitch", true, m_eSoundReload);

	shared_str integrated_gl			= READ_IF_EXISTS(pSettings, r_string, section, "grenade_launcher", "");
	if (integrated_gl.size())
		process_gl						(xr_new<CGrenadeLauncher>(this, integrated_gl), true);
}

void CWeaponMagazinedWGrenade::net_Destroy()
{
	inherited::net_Destroy				();
	stop_flame_particles_gl				();
}

bool CWeaponMagazinedWGrenade::switch_mode()
{
	if (!m_pLauncher || IsPending())
		return							false;

	m_bGrenadeMode						= !m_bGrenadeMode;
	m_BriefInfo_CalcFrame				= 0;
	if (HudAnimationExist("anm_switch"))
	{
		setADS							(0);
		SetPending						(TRUE);
		SwitchState						(eSwitch);
	}
	else
		PlayAnimIdle					();

	return								true;
}

LPCSTR CWeaponMagazinedWGrenade::get_anm_prefix() const
{
	return								(m_bGrenadeMode) ? "g" : inherited::get_anm_prefix();
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags)
{
	if (m_bGrenadeMode && cmd == kWPN_FIRE)
	{
		if (IsPending())
			return						false;

		if (flags&CMD_START)
		{
			if (m_grenade)
				launch_grenade			();
			else
				OnEmptyClick			();
		}
		return							true;
	}
	if (inherited::Action(cmd, flags))
		return							true;

	switch (cmd)
	{
	case kWPN_FUNC:
		if (flags&CMD_START && !IsPending())
			return						switch_mode();
	}
	return								false;
}

void CWeaponMagazinedWGrenade::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent					(P, type);
	if (type == GE_LAUNCH_ROCKET)
	{
		u16								id;
		P.r_u16							(id);
		DetachRocket					(id, true);
		
		PlayAnimShoot();
		PlaySound("sndShotG", fire_point_gl());
		AddShotEffector();
		start_flame_particles_gl();
		CCartridge l_cartridge;
		m_grenade->Get(l_cartridge);
		appendRecoil(m_fLaunchSpeed * l_cartridge.param_s.fBulletMass);
	}
}

void CWeaponMagazinedWGrenade::launch_grenade()
{
	Fvector p1, d;
	auto E = smart_cast<CEntity*>(H_Parent());
	E->g_fireParams(this, p1, d);
	p1.set(fire_point_gl());

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
			Fvector Transference;
			Transference.mul(d, RQ.range);
			Fvector					res[2];
			u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference,
				CRocketLauncher::m_fLaunchSpeed,
				EffectiveGravity(),
				res);

			if (canfire0 != 0)
				d = res[0];
		}
	}

	d.normalize();
	d.mul(CRocketLauncher::m_fLaunchSpeed);

	Fmatrix launch_matrix = Fidentity;
	launch_matrix.k.set(d);
	Fvector::generate_orthonormal_basis(launch_matrix.k,
		launch_matrix.j,
		launch_matrix.i);
	launch_matrix.c.set(p1);
	VERIFY2(_valid(launch_matrix), "CWeaponMagazinedWGrenade::SwitchState. Invalid launch_matrix!");
	CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

	CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
	VERIFY(pGrenade);
	pGrenade->SetInitiator(H_Parent()->ID());

	if (Local() && OnServer())
	{
		NET_Packet					P;
		u_EventGen(P, GE_LAUNCH_ROCKET, ID());
		P.w_u16(getCurrentRocket()->ID());
		u_EventSend(P);
	}
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S, u32 oldState)
{
	switch (S)
	{
	case eSwitch:
		PlayHUDMotion					("anm_switch", TRUE, eSwitch);
		PlaySound						("sndSwitch", get_LastFP());
		break;
	}

	inherited::OnStateSwitch			(S, oldState);
}

void CWeaponMagazinedWGrenade::PlayAnimReload()
{
	switch (m_sub_state)
	{
	case eSubstateReloadAttachG:
		m_pLauncher->m_slot->loadingAttach();
		PlayHUDMotion					("anm_attach_g", FALSE, GetState());
		if (m_sounds_enabled)
			PlaySound					("sndReloadG", fire_point_gl());
		break;
	case eSubstateReloadDetachG:
		PlayHUDMotion					("anm_detach_g", TRUE, GetState());
		break;
	default:
		inherited::PlayAnimReload		();
	}
}

void CWeaponMagazinedWGrenade::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eSwitch:
		SwitchState						(eIdle);
		break;
	case eReload:
		switch (m_sub_state)
		{
		case eSubstateReloadAttachG:
			m_pLauncher->m_slot->finishLoading();
			SwitchState					(eIdle);
			break;
		case eSubstateReloadDetachG:
			m_pLauncher->m_slot->loadingDetach();
			if (m_pLauncher->m_slot->isLoading())
			{
				m_sub_state				= eSubstateReloadAttachG;
				PlayAnimReload			();
			}
			else
				SwitchState				(eIdle);
			break;
		default:
			inherited::OnAnimationEnd	(state);
		}
		break;
	default:
		inherited::OnAnimationEnd		(state);
	}
}

void CWeaponMagazinedWGrenade::UpdateSounds()
{
	inherited::UpdateSounds				();
	m_sounds.SetPosition				("sndShotG", fire_point_gl());
	m_sounds.SetPosition				("sndReloadG", fire_point_gl());
	m_sounds.SetPosition				("sndSwitch", fire_point_gl());
}

bool CWeaponMagazinedWGrenade::IsNecessaryItem(const shared_str& item_sect)
{
	bool res							= inherited::IsNecessaryItem(item_sect);
	if (res || !m_pLauncher)
		return							res;

	auto slot_type						= READ_IF_EXISTS(pSettings, r_string, item_sect, "slot_type", 0);
	return								(m_pLauncher->m_slot->type == slot_type);
}

void CWeaponMagazinedWGrenade::process_gl(CGrenadeLauncher* gl, bool attach)
{
	if (m_bGrenadeMode)
		switch_mode						();

	m_pLauncher							= (attach) ? gl : nullptr;
	gl->m_wpn							= (attach) ? this : nullptr;

	if (attach)
	{
		m_hud->ProcessGL				(gl);
		m_fLaunchSpeed					= gl->GetGrenadeVel();
		m_flame_particles_gl_name		= gl->FlameParticles();
		m_sounds.LoadSound				(*gl->O.cNameSect(), "snd_shoot_grenade", "sndShotG", true, m_eSoundShot);

		Dmatrix trans					= gl->m_slot->model_offset;
		if (auto addon = gl->cast<CAddon*>())
			trans.mulA_43				(addon->getLocalTransform());
		m_muzzle_point_gl				= static_cast<Fvector>(trans.c);
	}
}

Fvector CR$ CWeaponMagazinedWGrenade::fire_point_gl()
{
	if (m_fire_point_gl_update_frame != Device.dwFrame && m_pLauncher)
	{
		auto hi							= HudItemData();
		Fmatrix trans					= (hi) ? static_cast<Fmatrix>(hi->m_transform) : XFORM();
		trans.transform_tiny			(m_fire_point_gl, m_muzzle_point_gl);
		m_fire_point_gl_update_frame	= Device.dwFrame;
	}
	return								m_fire_point_gl;
}

float CWeaponMagazinedWGrenade::Aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnChild:
	{
		CObject* obj					= (CObject*)data;
		if (auto rocket = obj->Cast<CCustomRocket*>())
		{
			if (param)
				AttachRocket			(obj->ID(), this);
			else
				DetachRocket			(obj->ID(), false);
		}
		break;
	}
	case eWeight:
		return							inherited::Aboba(type, data, param);		//--xd need implement grenade weight when reworking underbarrel gls
	case eSyncData:
	{
		float res						= inherited::Aboba(type, data, param);
		auto se_obj						= (CSE_ALifeDynamicObject*)data;
		auto se_wpn						= smart_cast<CSE_ALifeItemWeaponMagazinedWGL*>(se_obj);
		if (param)
			se_wpn->m_bGrenadeMode		= (u8)m_bGrenadeMode;
		else
			m_bGrenadeMode				= !!se_wpn->m_bGrenadeMode;
		return							res;
	}
	case eOnAddon:
	{
		auto addon						= (CAddon*)data;
		if (auto grenade = addon->cast<CWeaponAmmo*>())
		{
			m_grenade					= (param) ? grenade : nullptr;
			if (m_grenade && !getRocketCount())
			{
				shared_str fake_grenade_name = pSettings->r_string(m_grenade->cNameSect(), "fake_grenade_name");
				CRocketLauncher::SpawnRocket(fake_grenade_name, this);
			}
		}
		break;
	}
	}

	return								inherited::Aboba(type, data, param);
}

bool CWeaponMagazinedWGrenade::HasAltAim() const
{
	return								(m_bGrenadeMode) ? false : inherited::HasAltAim();
}

int CWeaponMagazinedWGrenade::ADS() const
{
	int res								= inherited::ADS();
	if (m_bGrenadeMode && res)
		res								= 2;
	return								res;
}

void CWeaponMagazinedWGrenade::start_flame_particles_gl()
{
	CShootingObject::StartParticles		(m_flame_particles_gl, *m_flame_particles_gl_name, fire_point_gl());
}

void CWeaponMagazinedWGrenade::stop_flame_particles_gl()
{
	CShootingObject::StopParticles		(m_flame_particles_gl);
}

void CWeaponMagazinedWGrenade::update_flame_particles_gl()
{
	if (m_flame_particles_gl)
		CShootingObject::UpdateParticles(m_flame_particles_gl, fire_point_gl());
}

void CWeaponMagazinedWGrenade::UpdateCL()
{
	inherited::UpdateCL					();
	update_flame_particles_gl			();
}

void CWeaponMagazinedWGrenade::process_addon_modules(CGameObject& obj, bool attach)
{
	if (auto gl = obj.Cast<CGrenadeLauncher*>())
		process_gl						(gl, attach);
	inherited::process_addon_modules	(obj, attach);
}
