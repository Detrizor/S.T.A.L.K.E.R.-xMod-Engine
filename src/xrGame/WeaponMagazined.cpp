#include "pch_script.h"

#include "WeaponMagazined.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "GrenadeLauncher.h"
#include "inventory.h"
#include "InventoryOwner.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "UIGameCustom.h"
#include "object_broker.h"
#include "string_table.h"
#include "MPPlayersBag.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "HudSound.h"
#include "Magazine.h"
#include "player_hud.h"
#include "../../xrEngine/xr_input.h"
#include "../build_config_defines.h"

#include "weapon_hud.h"
#include "scope.h"
#include "silencer.h"
#include "addon.h"
#include "UI.h"
#include "Level_Bullet_Manager.h"
#include "foldable.h"

ENGINE_API	bool	g_dedicated_server;

CWeaponMagazined::CWeaponMagazined(ESoundTypes eSoundType) : CWeapon()
{
	m_eSoundShow = ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide = ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundFiremode = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	m_sounds_enabled = true;
	m_sSndShotCurrent = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_iQueueSize = -1;
	m_bHasDifferentFireModes = false;
	m_iCurFireMode = -1;

	m_barrel_len = 0.f;
}

CWeaponMagazined::~CWeaponMagazined()
{
	xr_delete(m_hud);
}

void CWeaponMagazined::net_Destroy()
{
	inherited::net_Destroy();
}

void CWeaponMagazined::Load(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	m_sounds.LoadSound					(section, "snd_empty", "sndEmptyClick", true, m_eSoundEmptyClick);

	shared_str snd						= "snd_shoot";
	m_layered_sounds.LoadSound			(section, *snd, "sndShot", false, m_eSoundShot);
	m_layered_sounds.LoadSound			(section, (pSettings->line_exist(section, "snd_shoot_actor")) ? "snd_shoot_actor" : *snd, "sndShotActor", false, m_eSoundShot);
	if (pSettings->line_exist(section, "snd_silncer_shot"))
		snd								= "snd_silncer_shot";
	m_layered_sounds.LoadSound			(section, *snd, "sndSilencerShot", false, m_eSoundShot);
	if (pSettings->line_exist(section, "snd_silncer_shot_actor"))
		snd								= "snd_silncer_shot_actor";
	m_layered_sounds.LoadSound			(section, *snd, "sndSilencerShotActor", false, m_eSoundShot);

	m_sounds.LoadSound					(*HudSection(), "snd_draw", "sndShow", true, m_eSoundShow);
	m_sounds.LoadSound					(*HudSection(), "snd_holster", "sndHide", true, m_eSoundHide);
	m_sounds.LoadSound					(*HudSection(), "snd_firemode", "sndFiremode", true, m_eSoundFiremode);
	m_sounds.LoadSound					(*HudSection(), "snd_reload", "sndReload", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_detach", "sndDetach", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_attach", "sndAttach", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_bolt_pull", "sndBoltPull", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_bolt_release", "sndBoltRelease", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_bolt_lock", "sndBoltLock", true, m_eSoundReload); 
	m_sounds.LoadSound					(*HudSection(), "snd_load_chamber", "sndLoadChamber", true, m_eSoundReload);

	if (pSettings->line_exist(section, "base_dispersioned_bullets_count"))
	{
		m_iBaseDispersionedBulletsCount	= pSettings->r_u8(section, "base_dispersioned_bullets_count");
		m_base_dispersion_shot_time		= 60.f / pSettings->r_float(section,"base_dispersioned_rpm");
	}

	if (pSettings->line_exist(section, "fire_modes"))
	{
		m_bHasDifferentFireModes = true;
		shared_str FireModesList = pSettings->r_string(section, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_aFireModes.clear();

		for (int i = 0; i < ModesCount; i++)
		{
			string16 sItem;
			_GetItem(FireModesList.c_str(), i, sItem);
			m_aFireModes.push_back((s8) atoi(sItem));
		}

		m_iCurFireMode = ModesCount - 1;
	}
	else
		m_bHasDifferentFireModes = false;

	m_hud								= xr_new<CWeaponHud>(this);

	if (auto ao = getModule<MAddonOwner>())
	{
		for (auto& s : ao->AddonSlots())
		{
			if (s->attach == "magazine")
				m_magazine_slot			= s.get();
			if (s->attach == "muzzle")
				s->model_offset.translate_add(static_cast<Dvector>(m_fire_point));
		}
	}

	m_animation_slot_reloading			= READ_IF_EXISTS(pSettings, r_u32, section, "animation_slot_reloading", m_animation_slot);
	m_lock_state_reload					= !!READ_IF_EXISTS(pSettings, r_bool, section, "lock_state_reload", FALSE);
	m_lock_state_shooting				= !!READ_IF_EXISTS(pSettings, r_bool, section, "lock_state_shooting", FALSE);
	m_mag_attach_bolt_release			= !!READ_IF_EXISTS(pSettings, r_bool, section, "mag_attach_bolt_release", FALSE);
	m_bolt_catch						= !!READ_IF_EXISTS(pSettings, r_bool, section, "bolt_catch", FALSE);

	LPCSTR integrated_addons			= READ_IF_EXISTS(pSettings, r_string, section, "integrated_addons", 0);
	for (int i = 0, cnt = _GetItemCount(integrated_addons); i < cnt; i++)
	{
		string64						addon_sect;
		_GetItem						(integrated_addons, i, addon_sect);
		MAddon::addAddonModules			(*this, addon_sect);
		process_addon_data				(*this, addon_sect, true);
	}
	process_addon_data					(*this, cNameSect(), true);
	process_addon_modules				(*this, true);

	if (pSettings->line_exist(hud_sect, "empty_click_anm"))
	{
		m_empty_click_anm.name			= pSettings->r_string(hud_sect, "empty_click_anm");
		m_empty_click_anm.speed			= pSettings->r_float(hud_sect, "empty_click_anm_speed");
		m_empty_click_anm.power			= pSettings->r_float(hud_sect, "empty_click_anm_power");
	}

	if (pSettings->line_exist(hud_sect, "firemode_anm"))
	{
		m_firemode_anm.name				= pSettings->r_string(hud_sect, "firemode_anm");
		m_firemode_anm.speed			= pSettings->r_float(hud_sect, "firemode_anm_speed");
		m_firemode_anm.power			= pSettings->r_float(hud_sect, "firemode_anm_power");
	}
	
	hud_sect							= pSettings->r_string(cNameSect(), (m_grip) ? "hud" : "hud_unusable");
}

void CWeaponMagazined::FireStart()
{
	if (!IsMisfire())
	{
		if (has_ammo_to_shoot() && m_barrel_len)
		{
			if (!IsWorking() || AllowFireWhileWorking())
			{
				if (GetState() == eReload) return;
				if (GetState() == eShowing) return;
				if (GetState() == eHiding) return;
				if (GetState() == eMisfire) return;

				inherited::FireStart();

				R_ASSERT(H_Parent());
				SwitchState(eFire);
			}
		}
		else if (eReload != GetState())
			OnMagazineEmpty();
	}
	else
	{//misfire
		//Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
		CGameObject	*object = smart_cast<CGameObject*>(H_Parent());
		if (object)
			object->callback(GameObject::eOnWeaponJammed)(object->lua_game_object(), this->lua_game_object());
#endif
		//-Alundaio

		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
			CurrentGameUI()->AddCustomStatic("gun_jammed", true);

		OnEmptyClick();
	}
}

bool CWeaponMagazined::has_ammo_for_reload(int count) const
{
	return								(unlimited_ammo()) ? true : (m_current_ammo && m_current_ammo->GetAmmoCount() >= count);
}

void CWeaponMagazined::Reload()
{
	if (!m_pInventory && GetState() != eIdle)
	{
		SwitchState						(eIdle);
		return;
	}

	if (IsMisfire() || m_actor && m_chamber.capacity() && (!m_lock_state_shooting || !m_magazine || m_magazine->Empty()))
	{
		bool lock						= !m_locked && (m_lock_state_shooting || m_bolt_catch && HudAnimationExist("anm_bolt_lock") && !get_cartridge_from_mag(m_cartridge));
		StartReload						((lock) ? eSubstateReloadBoltLock : eSubstateReloadBolt);
	}
	else if (!m_actor && has_ammo_for_reload())
		StartReload						(HudAnimationExist("anm_detach") ? eSubstateReloadDetach : eSubstateReloadBegin);
}

void CWeaponMagazined::StartReload(EWeaponSubStates substate)
{
	if (GetState() == eReload)
		return;
	
	bool lock							= substate != eSubstateReloadBoltLock && m_lock_state_reload && isEmptyChamber() && !m_locked;
	if (m_actor && lock)
	{
		auto mag						= m_magazine_slot->getLoadingAddon();
		if (!mag || mag->O.getModule<MMagazine>()->Empty())
			lock						= false;
	}
	m_sub_state							= (lock) ? eSubstateReloadBoltLock : substate;

	SwitchState							(eReload);
}

bool CWeaponMagazined::discharge(CCartridge& destination, bool with_chamber)
{
	if (m_actor)
	{
#ifdef	EXTENDED_WEAPON_CALLBACKS
		int	AC = GetSuitableAmmoTotal();
		m_actor->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
#endif
	}

	if (with_chamber && m_chamber.size())
	{
		reload_chamber					(&destination);
		return							true;
	}

	if (m_magazin.size())
	{
		CCartridge& back_cartridge		= m_magazin.back();
		destination.m_ammoSect			= back_cartridge.m_ammoSect;
		destination.m_fCondition		= back_cartridge.m_fCondition;
		m_magazin.pop_back				();
		return							true;
	}
	
	return								false;
}

bool CWeaponMagazined::canTake(CWeaponAmmo CPC ammo, bool chamber) const
{
	if (chamber && !m_lock_state_shooting || !m_magazine_slot)
	{
		auto& storage					= (chamber) ? m_chamber : m_magazin;
		if (storage.size() < storage.capacity())
		{
			for (auto& t : m_ammoTypes)
				if (t == ammo->Section())
					return				true;
		}
	}
	return								false;
}

void CWeaponMagazined::OnMagazineEmpty()
{
#ifdef	EXTENDED_WEAPON_CALLBACKS
	if (m_actor)
	{
		int	AC = GetSuitableAmmoTotal();
		m_actor->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
	}
#endif
	
	OnEmptyClick();
}

LPCSTR CWeaponMagazined::anmType() const
{
	if ((m_sub_state == eSubstateReloadBolt || m_sub_state == eSubstateReloadBoltLock) && !m_shot_shell && m_chamber.empty())
		return							"_dummy";
	return								(m_locked) ? "_empty" : inherited::anmType();
}

u32 CWeaponMagazined::animation_slot() const
{
	if (GetState() == eReload)
		return							m_animation_slot_reloading;
	return								inherited::animation_slot();
}

CCartridge CWeaponMagazined::getCartridgeToShoot()
{
	bool expand							= !(m_actor && m_actor->unlimited_ammo());

	CCartridge							res;
	if (m_chamber.capacity() && !m_lock_state_shooting)
	{
		res								= m_chamber.back();
		if (expand)
		{
			m_chamber.pop_back			();
			m_shot_shell				= true;
			if (fOneShotTime != 0.f)
				reload_chamber			();
		}
	}
	else
		get_cartridge_from_mag			(res, expand);
	return								res;
}

bool CWeaponMagazined::get_cartridge_from_mag(CCartridge& dest, bool expand)
{
	if (m_magazine && !m_magazine->Empty())
		m_magazine->GetCartridge		(dest, expand);
	else if (!m_magazin.empty())
	{
		dest							= m_magazin.back();
		if (expand)
			m_magazin.pop_back			();
	}
	else
		return							false;
	return								true;
}

void CWeaponMagazined::reload_chamber(CCartridge* dest)
{
	if (!m_chamber.empty())
	{
		CCartridge& cartridge			= m_chamber.back();
		if (dest)
			*dest						= cartridge;
		else
		{
			CInventoryOwner* IO			= Parent->scast<CInventoryOwner*>();
			IO->O->giveItem				(*cartridge.m_ammoSect, cartridge.m_fCondition);
		}
		m_chamber.pop_back				();
	}
	
	load_chamber						(true);
}

void CWeaponMagazined::load_chamber(bool from_mag)
{
	bMisfire							= false;
	m_locked							= false;
	m_shot_shell						= false;

	if (from_mag && !get_cartridge_from_mag(m_cartridge, true))
		return;

	m_chamber.push_back					(m_cartridge);
}

void CWeaponMagazined::loadChamber(CWeaponAmmo* ammo)
{
	if (ammo)
		ammo->Get						(m_cartridge);
	load_chamber						(false);
}

void CWeaponMagazined::initReload(CWeaponAmmo* ammo)
{
	m_current_ammo						= ammo;
	StartReload							(eSubstateReloadBegin);
}

void CWeaponMagazined::onFold(MFoldable CP$ foldable, bool new_status)
{
	if (auto scope = foldable->O.getModule<MScope>())
		process_scope					(scope, !new_status);
	process_align_front					(&foldable->O, !new_status);
}

bool CWeaponMagazined::reloadCartridge()
{
	m_BriefInfo_CalcFrame				= 0;
	if (m_magazin.size() < m_magazin.capacity() && (unlimited_ammo() || m_current_ammo && m_current_ammo->Get(m_cartridge)))
	{
		m_magazin.push_back				(m_cartridge);
		if (m_current_ammo && m_current_ammo->object_removed())
			m_current_ammo				= nullptr;
		return							true;
	}
	return								false;
}

void CWeaponMagazined::ReloadMagazine()
{
	while (reloadCartridge());
}

void CWeaponMagazined::OnStateSwitch(u32 S, u32 oldState)
{
	inherited::OnStateSwitch(S, oldState);
	CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
	switch (S)
	{
	case eIdle:
		switch2_Idle();
		break;
	case eFire:
		switch2_Fire();
		break;
	case eMisfire:
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
			CurrentGameUI()->AddCustomStatic("gun_jammed", true);
		break;
	case eReload:
		if (owner)
			m_sounds_enabled = owner->CanPlayShHdRldSounds();
		switch2_Reload();
		break;
	case eShowing:
		if (owner)
			m_sounds_enabled = owner->CanPlayShHdRldSounds();
		switch2_Showing();
		break;
	case eHiding:
		if (owner)
			m_sounds_enabled = owner->CanPlayShHdRldSounds();

		if (oldState != eHiding)
			switch2_Hiding();
		break;
	case eHidden:
		switch2_Hidden();
		break;
	case eFiremode:
		on_firemode_switch();
		break;
	}
}

void CWeaponMagazined::UpdateCL()
{
	inherited::UpdateCL();
	float dt = Device.fTimeDelta;

	switch (GetState())
	{
	case eShowing:
	case eHiding:
	case eReload:
	case eIdle:
	{
		fShotTimeCounter -= dt;
		clamp(fShotTimeCounter, 0.0f, flt_max);
		break;
	}
	case eFire:
		state_Fire(dt);
		break;
	case eMisfire:
		state_Misfire(dt);
		break;
	case eHidden:
		break;
	}

	UpdateSounds();
	updateRecoil();

	if (isEmptyChamber() && fIsZero(m_recoil_hud_impulse.magnitude()) && GetAmmoElapsed() && GetState() == eIdle && is_auto_bolt_allowed())
		StartReload(eSubstateReloadBolt);
}

void CWeaponMagazined::UpdateSounds()
{
	if (Device.dwFrame == dwUpdateSounds_Frame)
		return;

	dwUpdateSounds_Frame = Device.dwFrame;

	Fvector P = get_LastFP();
	m_sounds.SetPosition("sndShow", P);
	m_sounds.SetPosition("sndHide", P);
	m_sounds.SetPosition("sndReload", P);
	m_sounds.SetPosition("sndDetach", P);
	m_sounds.SetPosition("sndAttach", P);
	m_sounds.SetPosition("sndBoltPull", P);
	m_sounds.SetPosition("sndBoltRelease", P);
	m_sounds.SetPosition("sndBoltLock", P);
	m_sounds.SetPosition("sndLoadChamber", P);
}

void CWeaponMagazined::state_Fire(float dt)
{
	if (has_ammo_to_shoot())
	{
		if (!H_Parent())
		{
			StopShooting();
			return;
		}

		CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
		if (!io->inventory().ActiveItem())
		{
			StopShooting();
			return;
		}

		CEntity* E = smart_cast<CEntity*>(H_Parent());
		if (!E->g_stateFire())
		{
			StopShooting();
			return;
		}

		while (has_ammo_to_shoot() && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
		{
			if (CheckForMisfire())
			{
				if (m_lock_state_shooting)
				{
					m_chamber.push_back(getCartridgeToShoot());
					m_locked = false;
				}
				StopShooting();
				return;
			}

			++m_iShotNum;
			FireTrace();
			OnShot();

			m_bFireSingleShot = false;
			fShotTimeCounter = (m_iShotNum < m_iBaseDispersionedBulletsCount) ? m_base_dispersion_shot_time : fOneShotTime;
		}

		if (m_iShotNum == m_iQueueSize)
			m_bStopedAfterQueueFired = true;

		UpdateSounds();
	}

	if (fShotTimeCounter < 0)
	{
		if (!has_ammo_to_shoot())
			OnMagazineEmpty();

		StopShooting();
	}
	else
		fShotTimeCounter -= dt;
}

void CWeaponMagazined::state_Misfire(float dt)
{
	OnEmptyClick();
	SwitchState(eIdle);

	bMisfire = true;

	UpdateSounds();
}

void CWeaponMagazined::SetDefaults()
{
	CWeapon::SetDefaults();
}

void CWeaponMagazined::OnShot()
{
	m_layered_sounds.PlaySound(*m_sSndShotCurrent, get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);

	// Camera
	AddShotEffector();

	// Animation
	PlayAnimShoot();

	// Shell Drop
	Fvector vel;
	PHGetLinearVell(vel);
	OnShellDrop(get_LastSP(), vel);

	// Огонь из ствола
	StartFlameParticles();

	//дым из ствола
	ForceUpdateFireParticles();
	StartSmokeParticles(get_LastFP(), vel);
}

void CWeaponMagazined::OnEmptyClick()
{
	PlaySound("sndEmptyClick", get_LastFP());
	playBlendAnm(m_empty_click_anm);
	if (m_lock_state_shooting && m_locked)
	{
		m_locked = false;
		PlaySound("sndBoltRelease", get_LastFP());
		PlayHUDMotion("anm_shoot", FALSE, GetState());
		return;
	}
	PlayHUDMotion("anm_trigger", FALSE, GetState());
}

void CWeaponMagazined::switch2_Idle()
{
	m_iShotNum = 0;
	m_sub_state = eSubstateReloadBegin;
	PlayAnimIdle();
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire()
{
	CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
	if (!io)
		return;

	CInventoryItem* ii = smart_cast<CInventoryItem*>(this);
	if (ii != io->inventory().ActiveItem())
	{
		Msg("WARNING: Not an active item, item %s, owner %s, active item %s", *cName(), *H_Parent()->cName(), io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");	
		return;
	}
#ifdef DEBUG
	if (!(io && (ii == io->inventory().ActiveItem())))
	{
		CAI_Stalker			*stalker = smart_cast<CAI_Stalker*>(H_Parent());
		if (stalker)
		{
			stalker->planner().show();
			stalker->planner().show_current_world_state();
			stalker->planner().show_target_world_state();
		}
	}
#endif // DEBUG

	m_bStopedAfterQueueFired = false;
	m_bFireSingleShot = true;
	m_iShotNum = 0;

	if ((OnClient() || Level().IsDemoPlay()) && !IsWorking())
		FireStart();
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();
	SetPending(TRUE);
	PlayAnimReload();
}

void CWeaponMagazined::switch2_Hiding()
{
	setAiming(false);
	CWeapon::FireEnd();
	
	SetPending(TRUE);

	m_sub_state = eSubstateReloadBegin;
	if (m_sounds_enabled)
		PlaySound("sndHide", get_LastFP());
}

void CWeaponMagazined::switch2_Hidden()
{
	CWeapon::FireEnd();

	StopCurrentAnimWithoutCallback();

	signal_HideComplete();
	RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
	SetPending(TRUE);
	if (m_sounds_enabled)
		PlaySound("sndShow", get_LastFP());
}

bool CWeaponMagazined::Action(u16 cmd, u32 flags)
{
	if (!m_grip)
		return false;

	if (m_hud->Action(cmd, flags))
		return true;

	if (inherited::Action(cmd, flags))
		return true;

	//если оружие чем-то занято, то ничего не делать
	if (IsPending()) return false;

	switch (cmd)
	{
	case kWPN_RELOAD:
		if (flags&CMD_START)
			Reload();
		return true;

	case kWPN_FIREMODE_PREV:
		if (flags&CMD_START)
		{
			OnPrevFireMode();
			return true;
		}
		break;

	case kWPN_FIREMODE_NEXT:
		if (flags&CMD_START)
		{
			OnNextFireMode();
			return true;
		}
		break;
	}
	return false;
}

void CWeaponMagazined::OnNextFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
	SwitchState(eFiremode);
}

void CWeaponMagazined::OnPrevFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
	SwitchState(eFiremode);
}

void CWeaponMagazined::SetQueueSize(int size)
{
	m_iQueueSize = size;
}

float CWeaponMagazined::GetWeaponDeterioration()
{
	return (m_iShotNum == 1) ? conditionDecreasePerShot : conditionDecreasePerQueueShot;
}

#include "string_table.h"
#include "ui\UIHudStatesWnd.h"
void CWeaponMagazined::GetBriefInfo(SWpnBriefInfo& info)
{
	VERIFY								(m_pInventory);

	MScope* scope						= getActiveScope();
	if (!scope)
		scope							= m_selected_scopes[0];
	if (!scope)
		scope							= m_selected_scopes[1];

	if (scope)
		info.zeroing.printf				("%d %s", scope->Zeroing(), CStringTable().translate("st_m").c_str());
	else
		info.zeroing					= "";

	if (scope && scope->Type() != MScope::eIS)
	{
		float magnification				= scope->GetCurrentMagnification();
		info.magnification.printf		(fEqual(round(magnification), magnification) ? "%.0fx" : "%.1fx", magnification);
	}
	else
		info.magnification				= "";

	if (HasFireModes())
	{
		if (m_iQueueSize < 0)
			info.fire_mode				= "A";
		else
			info.fire_mode.printf		("%d", m_iQueueSize);
	}
	else
		info.fire_mode					= "";

	if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
		return;

	GetSuitableAmmoTotal();//update m_BriefInfo_CalcFrame
}

bool CWeaponMagazined::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);

	LPCSTR str;
	bool result2 = process_if_exists(section, "fire_modes", str, test);
	if (result2 && !test)
	{
		int ModesCount = _GetItemCount(str);
		m_aFireModes.clear();
		for (int i = 0; i < ModesCount; ++i)
		{
			string16 sItem;
			_GetItem(str, i, sItem);
			m_aFireModes.push_back((s8) atoi(sItem));
		}
		m_iCurFireMode = ModesCount - 1;
	}
	result |= result2;

	result |= process_if_exists(section, "base_dispersioned_bullets_count", m_iBaseDispersionedBulletsCount, test);
	if (result2 = process_if_exists(section, "base_dispersioned_rpm", m_base_dispersion_shot_time, test))
		m_base_dispersion_shot_time = 60.f / m_base_dispersion_shot_time;
	result |= result2;

	return result;
}

bool CWeaponMagazined::CanTrade() const
{
	if (GetAmmoElapsed())
		return false;

	if (auto ao = getModule<MAddonOwner>())
	{
		for (auto& slot : ao->AddonSlots())
		{
			if (!slot->addons.empty())
				return false;
		}
	}

	return true;
}

void CWeaponMagazined::OnHiddenItem()
{
	if (m_magazine_slot && m_magazine_slot->isLoading())
		m_magazine_slot->finishLoading	(true);
	inherited::OnHiddenItem				();
}

bool CWeaponMagazined::is_auto_bolt_allowed() const
{
	return !m_actor;
}

bool CWeaponMagazined::has_ammo_to_shoot()
{
	if (m_lock_state_shooting)
		return							(m_locked || !m_actor) && get_cartridge_from_mag(m_cartridge);
	return								(m_chamber.capacity()) ? m_chamber.size() : get_cartridge_from_mag(m_cartridge);
}

bool CWeaponMagazined::is_detaching() const
{
	return								(m_magazine_slot && !m_magazine_slot->isLoading());
}

bool CWeaponMagazined::isEmptyChamber() const
{
	return								(m_chamber.capacity() && m_chamber.empty());
}

void CWeaponMagazined::modify_holder_params C$(float& range, float& fov)
{
	for (auto s : m_attached_scopes)
	{
		if (s->Type() == MScope::eOptics)
		{
			s->modify_holder_params		(range, fov);
			return;
		}
	}
}

MScope* CWeaponMagazined::getActiveScope() const
{
	if (ADS() == 1)
		return							m_selected_scopes[0];
	else if (ADS() == -1)
		return							m_selected_scopes[1];
	return								nullptr;
}

void CWeaponMagazined::ZoomInc()
{
	MScope* scope						= getActiveScope();
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && scope)
		scope->ZoomChange				(1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange		(1);
	}
	else if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
	{
		if (ADS())
			cycle_scope					(int(ADS() == -1), true);
	}
	else if (pInput->iGetAsyncKeyState(DIK_RALT))
	{
		if (ADS() && fLess(m_ads_shift, s_ads_shift_max))
			m_ads_shift					+= s_ads_shift_step;
	}
	else if (pInput->iGetAsyncKeyState(DIK_RCONTROL))
	{
		if (scope)
			if (scope->reticleChange(1))
				on_reticle_switch		();
	}
	else
		inherited::ZoomInc				();
}

void CWeaponMagazined::ZoomDec()
{
	MScope* scope						= getActiveScope();
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && scope)
		scope->ZoomChange				(-1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange		(-1);
	}
	else if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
	{
		if (ADS())
			cycle_scope					(int(ADS() == -1), false);
	}
	else if (pInput->iGetAsyncKeyState(DIK_RALT))
	{
		if (ADS() && fMore(m_ads_shift, -s_ads_shift_max))
			m_ads_shift					-= s_ads_shift_step;
	}
	else if (pInput->iGetAsyncKeyState(DIK_RCONTROL))
	{
		if (scope)
			if (scope->reticleChange(-1))
				on_reticle_switch		();
	}
	else
		inherited::ZoomDec				();
}

bool CWeaponMagazined::need_renderable()
{
	MScope* scope = getActiveScope();
	if (!scope || scope->Type() != MScope::eOptics || scope->isPiP())
		return true;
	return !IsRotatingToZoom();
}

bool CWeaponMagazined::render_item_ui_query()
{
	MScope* scope = getActiveScope();
	if (!scope || scope->Type() != MScope::eOptics)
		return false;

	if (scope->isPiP())
		return (Device.SVP.isRendering());

	return !IsRotatingToZoom();
}

void CWeaponMagazined::render_item_ui()
{
	getActiveScope()->RenderUI();
}

float CWeaponMagazined::CurrentZoomFactor(bool for_actor) const
{
	MScope* scope = getActiveScope();
	if (scope && scope->Type() == MScope::eOptics && (!scope->isPiP() && !IsRotatingToZoom() || !for_actor))
		return scope->GetCurrentMagnification();
	else
		return inherited::CurrentZoomFactor(for_actor);
}

void CWeaponMagazined::cycle_scope(int idx, bool up)
{
	if (m_attached_scopes.empty())
		return;

	if (auto cur_scope = m_selected_scopes[idx])
	{
		cur_scope->setSelection			(s8_max);
		for (int i = 0, e = m_attached_scopes.size(); i < e; i++)
		{
			if (m_attached_scopes[i] == cur_scope)
			{
				if (up)
				{
					int next			= i + 1;
					if ((next < e) && (m_attached_scopes[next] == m_selected_scopes[!idx]))
						next++;
					m_selected_scopes[idx] = (next < e) ? m_attached_scopes[next] : NULL;
				}
				else
				{
					int next			= i - 1;
					if ((next >= 0) && m_attached_scopes[next] == m_selected_scopes[!idx])
						next--;
					m_selected_scopes[idx] = (next >= 0) ? m_attached_scopes[next] : NULL;
				}
				break;
			}
		}
	}
	else
	{
		if (m_attached_scopes.size() == 1 && m_attached_scopes[0] == m_selected_scopes[!idx])
			return;

		int								next;
		if (up)
		{
			next						= 0;
			if (m_attached_scopes[next] == m_selected_scopes[!idx])
				next++;
		}
		else
		{
			next						= m_attached_scopes.size() - 1;
			if (m_attached_scopes[next] == m_selected_scopes[!idx])
				next--;
		}

		m_selected_scopes[idx]			= m_attached_scopes[next];
	}

	if (m_selected_scopes[idx])
		m_selected_scopes[idx]->setSelection(idx);
}

void CWeaponMagazined::on_firemode_switch()
{
	bool anim_started					= false;
	if (HudAnimationExist("anm_firemode"))
	{
		PlayHUDMotion					("anm_firemode", TRUE, GetState());
		anim_started					= true;
	}
	if (m_firemode_anm.name.size())
	{
		playBlendAnm					(m_firemode_anm, GetState());
		anim_started					= true;
	}
	
	if (anim_started)
	{
		SetPending						(TRUE);
		if (m_sounds_enabled)
			PlaySound					("sndFiremode", get_LastFP());
	}
	else
		SwitchState						(eIdle);
}

void CWeaponMagazined::on_reticle_switch()
{
	playBlendAnm						(m_empty_click_anm);
}

void CWeaponMagazined::process_addon(MAddon* addon, bool attach)
{
	process_addon_data					(addon->O, addon->O.cNameSect(), attach);
	process_addon_modules				(addon->O, attach);
}

static LPCSTR read_root_type(CObject* obj, LPCSTR part)
{
	auto parent							= obj->H_Parent();
	R_ASSERT							(parent);
	if (pSettings->line_exist(parent->cNameSect(), part))
		return							pSettings->r_string(parent->cNameSect(), part);
	return								read_root_type(parent, part);
}

void CWeaponMagazined::process_foregrip(CGameObject& obj, shared_str CR$ section, bool attach)
{
	LPCSTR foregrip_type				= (attach) ? pSettings->r_string(section, "foregrip_type") : read_root_type(&obj, "foregrip_type");
	m_foregrip_recoil_pattern			= readRecoilPattern(foregrip_type);
	m_foregrip_accuracy_modifier		= readAccuracyModifier(foregrip_type);
	m_anm_prefix						= pSettings->r_string("anm_prefixes", foregrip_type);
}

void CWeaponMagazined::process_addon_data(CGameObject& obj, shared_str CR$ section, bool attach)
{
	if (pSettings->line_exist(section, "grip_type"))
	{
		if (m_grip = attach)
		{
			LPCSTR grip_type			= pSettings->r_string(section, "grip_type");
			m_grip_accuracy_modifier	= readAccuracyModifier(grip_type);
		}
		hud_sect						= pSettings->r_string(cNameSect(), (attach) ? "hud" : "hud_unusable");
	}

	if (pSettings->line_exist(section, "stock_type"))
	{
		LPCSTR stock_type				= (attach) ? pSettings->r_string(section, "stock_type") : read_root_type(&obj, "stock_type");
		m_stock_recoil_pattern			= readRecoilPattern(stock_type);
		m_stock_accuracy_modifier		= readAccuracyModifier(stock_type);
	}
	
	if (pSettings->line_exist(section, "foregrip_type"))
		process_foregrip				(obj, section, attach);

	if (float length = READ_IF_EXISTS(pSettings, r_float, section, "barrel_length", 0.f))
	{
		m_barrel_length					+= (attach) ? length : -length;
		m_barrel_len					= pow(m_barrel_length, s_barrel_length_power);
	}

	if (size_t mag_size = READ_IF_EXISTS(pSettings, r_u32, section, "ammo_mag_size", 0))
	{
		if (attach)
			m_magazin.reserve			(mag_size);
		else
		{
			if (auto owner = H_Parent()->scast<CInventoryOwner*>())
				owner->discharge		(getModule<CInventoryItem>(), false, true);
			else
				m_magazin.clear			();
			m_magazin.shrink_to_fit		();
		}
	}

	process_align_front					(&obj, attach);
}

void CWeaponMagazined::process_addon_modules(CGameObject& obj, bool attach)
{
	if (auto mag = obj.getModule<MMagazine>())
		m_magazine						= (attach) ? mag : nullptr;
	if (auto scope = obj.getModule<MScope>())
		process_scope					(scope, attach);
	if (auto muzzle = obj.getModule<MMuzzle>())
		process_muzzle					(muzzle, attach);
	if (auto sil = obj.getModule<CSilencer>())
		process_silencer				(sil, attach);
}

void CWeaponMagazined::process_scope(MScope* scope, bool attach)
{
	if (auto foldable = scope->O.getModule<MFoldable>())
		if (foldable->getStatus())
			return;

	if (attach)
		m_hud->calculateScopeOffset		(scope);

	if (attach)
	{
		m_attached_scopes.push_back		(scope);
		if (scope->getSelection() == -1)
		{
			for (int i = 0; i < 2; i++)
			{
				if (!m_selected_scopes[i] || m_selected_scopes[i]->Type() == MScope::eIS && scope->Type() != MScope::eIS)
				{
					m_selected_scopes[i] = scope;
					scope->setSelection	(i);
					break;
				}
			}
		}
		else
			m_selected_scopes[scope->getSelection()] = scope;
	}
	else
	{
		if (m_selected_scopes[0] == scope)
			cycle_scope					(0);
		else if (m_selected_scopes[1] == scope)
			cycle_scope					(1);
		m_attached_scopes.erase			(::std::find(m_attached_scopes.begin(), m_attached_scopes.end(), scope));
	}

	if (scope->getBackupSight())
		process_scope					(scope->getBackupSight(), attach);
}

static void check_addons(MAddonOwner* ao, CGameObject* obj, Dvector& align_front)
{
	for (auto& s : ao->AddonSlots())
	{
		for (auto a : s->addons)
		{
			if (auto addon_ao = a->O.getModule<MAddonOwner>())
				check_addons			(addon_ao, obj, align_front);
			if (&a->O != obj && pSettings->line_exist(a->O.cNameSect(), "align_front"))
			{
				Dvector al				= pSettings->r_dvector3(a->O.cNameSect(), "align_front");
				al.add					(a->getLocalTransform().c);
				if (al.z < align_front.z)
					align_front			= al;
			}
		}
	}
}

void CWeaponMagazined::process_align_front(CGameObject* obj, bool attach)
{
	if (!pSettings->line_exist(obj->cNameSect(), "align_front"))
		return;
	
	if (attach)
	{
		m_align_front					= pSettings->r_dvector3(obj->cNameSect(), "align_front");
		if (auto addon = obj->getModule<MAddon>())
			m_align_front.add			(addon->getLocalTransform().c);
	}
	else
		m_align_front					= READ_IF_EXISTS(pSettings, r_dvector3, cNameSect(), "align_front", dMax);
	
	if (auto ao = getModule<MAddonOwner>())
		check_addons					(ao, obj, m_align_front);

	for (auto scope : m_attached_scopes)
		if (scope->Type() == MScope::eIS)
			m_hud->calculateScopeOffset	(scope);
}

static MMuzzle* get_parent_muzzle(CObject* obj)
{
	auto parent							= obj->H_Parent();
	if (!parent)
		return							nullptr;
	if (auto muzzle = parent->getModule<MMuzzle>())
		return							muzzle;
	return								get_parent_muzzle(parent);
}

void CWeaponMagazined::process_muzzle(MMuzzle* muzzle, bool attach)
{
	if (auto src = (attach) ? muzzle : get_parent_muzzle(&muzzle->O))
	{
		m_muzzle_recoil_pattern			= src->getRecoilPattern();
		m_fire_point					= src->getFirePoint();
		m_flash_hider					= src->isFlashHider();
		m_muzzle_koefs.fire_dispersion	= src->getDispersionK();
		m_hud->calculateAimOffsets		();
	}
}

void CWeaponMagazined::process_silencer(CSilencer* sil, bool attach)
{
	if (m_silencer = attach)
		m_muzzle_koefs.bullet_speed		= (attach) ? sil->getBulletSpeedK() : 1.f;
	UpdateSndShot						();
}

float CWeaponMagazined::Aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnAddon:
	{
		process_addon					(static_cast<MAddon*>(data), !!param);
		break;
	}
	case eWeight:
		return							inherited::Aboba(type, data, param) + GetMagazineWeight();
	case eTransferAddon:
	{
		auto addon						= static_cast<MAddon*>(data);
		if (addon->O.getModule<MMagazine>())
		{
			m_magazine_slot->startReloading((param) ? addon : nullptr);
			StartReload					((m_magazine_slot->addons.empty()) ? eSubstateReloadAttach : eSubstateReloadDetach);
			return						1.f;
		}
		break;
	}
	case eUpdateSlotsTransform:
	{
		float res						= inherited::Aboba(type, data, param);
		if (auto scope = getActiveScope())
			if (scope->Type() == MScope::eOptics)
				scope->updateCameraLenseOffset();
		return							res;
	}
	case eSyncData:
	{
		float res						= inherited::Aboba(type, data, param);
		auto se_obj						= static_cast<CSE_ALifeDynamicObject*>(data);
		auto se_wpn						= smart_cast<CSE_ALifeItemWeaponMagazined*>(se_obj);
		if (param)
		{
			se_wpn->m_u8CurFireMode		= (u8)m_iCurFireMode;
			se_wpn->m_ads_shift			= m_ads_shift;
		}
		else
		{
			m_iCurFireMode				= se_wpn->m_u8CurFireMode;
			SetQueueSize				(GetCurrentFireMode());
			m_ads_shift					= se_wpn->m_ads_shift;
		}
		return							res;
	}
	}

	return								inherited::Aboba(type, data, param);
}

void CWeaponMagazined::UpdateSndShot()
{
	if (m_actor)
		m_sSndShotCurrent = (m_silencer) ? "sndSilencerShotActor" : "sndShotActor";
	else
		m_sSndShotCurrent = (m_silencer) ? "sndSilencerShot" : "sndShot";
}

void CWeaponMagazined::OnTaken()
{
	UpdateSndShot();
}

void CWeaponMagazined::UpdateHudAdditional(Dmatrix& trans)
{
	if (m_grip)
		m_hud->UpdateHudAdditional(trans);
	else
		inherited::UpdateHudAdditional(trans);
}

bool CWeaponMagazined::IsRotatingToZoom C$()
{
	return m_hud->IsRotatingToZoom();
}

float CWeaponMagazined::GetMagazineWeight() const
{
	float res							= 0.f;
	LPCSTR last_type					= NULL;
	float last_ammo_weight				= 0.f;
	for (auto& c : m_magazin)
	{
		if (last_type != c.m_ammoSect.c_str())
		{
			last_type					= c.m_ammoSect.c_str();
			last_ammo_weight			= c.Weight();
		}
		res								+= last_ammo_weight;
	}

	for (auto& c : m_chamber)
		res								+= c.Weight();

	return								res;
}

void CWeaponMagazined::updateShadersDataAndSVP(CCameraManager& camera) const
{
	MScope* scope						= getActiveScope();
	Device.SVP.setActive(scope && scope->isPiP());
	if (!scope || scope->Type() == MScope::eIS)
		return;
	
	static Fmatrix						cam_trans;
	static Fvector						cam_hpb;
	static Dvector						self_hpb;
	static Fvector						d_hpb;

	camera.camera_Matrix				(cam_trans);
	cam_trans.getHPB					(cam_hpb);
	HudItemData()->m_transform.getHPB	(self_hpb);
	d_hpb								= cam_hpb;
	d_hpb.sub							(static_cast<Fvector>(self_hpb));

	float fov_tan						= g_aim_fov_tan;
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	if (scope->Type() == MScope::eOptics)
	{
		hud_params.w					= scope->getLenseFovTan() / g_aim_fov_tan;
		float zoom_factor				= CurrentZoomFactor(false);
		Device.SVP.setZoom				(zoom_factor);
		fov_tan							/= zoom_factor;
		Device.SVP.setFOV				(atanf(fov_tan) / (.5f * PI / 180.f));
		
		Fvector pos						= static_cast<Fvector>(scope->getObjectiveOffset());
		cam_trans.transform_tiny		(pos);
		Device.SVP.setPosition			(pos);
	}
	else
		hud_params.w					= scope->GetReticleScale();

	float distance						= scope->Zeroing();

	float x_derivation					= distance * tanf(d_hpb.x);
	float y_derivation					= distance * tanf(d_hpb.y);

	float h								= 2.f * fov_tan * distance;
	hud_params.x						= x_derivation / h;
	hud_params.y						= y_derivation / h;
	hud_params.z						= d_hpb.z;
}

u16 CWeaponMagazined::Zeroing C$()
{
	MScope* active_scope				= getActiveScope();
	return								(active_scope) ? active_scope->Zeroing() : 100.f;
}

Fvector CWeaponMagazined::getFullFireDirection(CCartridge CR$ c)
{
	auto hi								= HudItemData();
	if (!hi)
		return							get_LastFD();

	float distance						= Zeroing();
	Fvector transference				= m_hud->getTransference(distance);
	static_cast<Fmatrix>(hi->m_transform).transform_dir(transference);

	float air_resistance_correction		= Level().BulletManager().CalcZeroingCorrection(c.param_s.fAirResistZeroingCorrection, distance);
	float speed							= m_barrel_len * m_muzzle_koefs.bullet_speed * c.param_s.bullet_speed_per_barrel_len * air_resistance_correction;

	Fvector								result[2];
	TransferenceAndThrowVelToThrowDir	(transference, speed, Level().BulletManager().GravityConst(), result);
	result[0].normalize					();

	return								result[0];
}

void CWeaponMagazined::setADS(int mode)
{
	bool prev							= !!ADS();
	inherited::setADS					(mode);
	bool cur							= !!ADS();
	if (prev == cur)
		return;

	if (cur)
	{
		//Alundaio: callback not sure why vs2013 gives error, it's fine
	#ifdef EXTENDED_WEAPON_CALLBACKS
		if (auto object = smart_cast<CGameObject*>(H_Parent()))
			object->callback(GameObject::eOnWeaponZoomIn)(object->lua_game_object(),this->lua_game_object());
	#endif
		//-Alundaio

		if (m_actor)
		{
			auto S						= smart_cast<CEffectorZoomInertion*>(m_actor->Cameras().GetCamEffector(eCEZoom));
			if (!S)
			{
				S						= (CEffectorZoomInertion*)m_actor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion>());
				S->Init					(this);
			}
			S->SetRndSeed				(m_actor->GetZoomRndSeed());
			R_ASSERT					(S);
		}
	}
	else
	{
		//Alundaio
	#ifdef EXTENDED_WEAPON_CALLBACKS
		if (auto object = smart_cast<CGameObject*>(H_Parent()))
			object->callback(GameObject::eOnWeaponZoomOut)(object->lua_game_object(), this->lua_game_object());
	#endif
		//-Alundaio

		if (m_actor)
			m_actor->Cameras().RemoveCamEffector(eCEZoom);
	}
}

void CWeaponMagazined::updateRecoil()
{
	if (bWorking && m_iShotNum < m_iBaseDispersionedBulletsCount)
		return;

	bool hud_impulse = !fIsZero(m_recoil_hud_impulse.magnitude());
	bool hud_shift = !fIsZero(m_recoil_hud_shift.magnitude());
	bool cam_impulse = !fIsZero(m_recoil_cam_impulse.magnitude());
	if (!hud_impulse && !hud_shift && !cam_impulse)
		return;

	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

	CEntityAlive* ea = smart_cast<CEntityAlive*>(H_Parent());
	float accuracy = (ea) ? ea->getAccuracy() : 1.f;
	accuracy *= m_layout_accuracy_modifier * m_grip_accuracy_modifier * m_stock_accuracy_modifier * m_foregrip_accuracy_modifier;

	if (hud_impulse || hud_shift)
	{
		float dt = fAvgTimeDelta / GetControlInertionFactor();
		auto update_hud_recoil_shift = [this, dt](Fvector4 CR$ impulse)
		{
			Fvector4 delta = impulse;
			delta.mul(dt);
			float prev_shift = m_recoil_hud_shift.magnitude();
			m_recoil_hud_shift.add(delta);
			return (fMoreOrEqual(delta.magnitude(), prev_shift));
		};

		if (hud_impulse)
		{
			update_hud_recoil_shift(m_recoil_hud_impulse);

			Fvector4 stopping_power = m_recoil_hud_impulse;
			stopping_power.normalize();
			stopping_power.mul(m_recoil_hud_shift.magnitude());
			stopping_power.mul(-s_recoil_hud_stopping_power_per_shift);
			stopping_power.mul(accuracy);
			stopping_power.mul(fAvgTimeDelta);
			if (fMoreOrEqual(stopping_power.magnitude(), m_recoil_hud_impulse.magnitude()))
				m_recoil_hud_impulse = vZero4;
			else
				m_recoil_hud_impulse.add(stopping_power);
		}
		else
		{
			Fvector4 relax_impulse = m_recoil_hud_shift;
			relax_impulse.mul(-s_recoil_hud_relax_impulse_per_shift);
			relax_impulse.mul(accuracy);
			if (update_hud_recoil_shift(relax_impulse))
				m_recoil_hud_shift = vZero4;
		}
	}

	if (cam_impulse)
	{
		m_recoil_cam_delta = m_recoil_cam_impulse;
		m_recoil_cam_delta.mul(s_recoil_cam_angle_per_delta);
		m_recoil_cam_delta.mul(fAvgTimeDelta);

		Fvector stopping_power = m_recoil_cam_impulse;
		stopping_power.mul(-s_recoil_cam_stopping_power_per_impulse);
		stopping_power.mul(accuracy);
		stopping_power.mul(fAvgTimeDelta);
		m_recoil_cam_impulse.add(stopping_power);
		if (fIsZero(m_recoil_cam_impulse.magnitude()) || fMoreOrEqual(m_recoil_cam_impulse.dotproduct(stopping_power), 0.f))
		{
			if (m_recoil_cam_last_impulse.magnitude())
			{
				m_recoil_cam_impulse = m_recoil_cam_last_impulse;
				m_recoil_cam_impulse.mul(-s_recoil_cam_relax_impulse_ratio);
				m_recoil_cam_last_impulse = vZero;
			}
			else
				m_recoil_cam_impulse = vZero;
		}
	}
}

float CWeaponMagazined::s_ads_shift_step;
float CWeaponMagazined::s_ads_shift_max;
float CWeaponMagazined::s_barrel_length_power;
float CWeaponMagazined::s_recoil_hud_stopping_power_per_shift;
float CWeaponMagazined::s_recoil_hud_relax_impulse_per_shift;
float CWeaponMagazined::s_recoil_cam_angle_per_delta;
float CWeaponMagazined::s_recoil_cam_stopping_power_per_impulse;
float CWeaponMagazined::s_recoil_cam_relax_impulse_ratio;

void CWeaponMagazined::loadStaticData()
{
	s_ads_shift_step					= pSettings->r_float("weapon_manager", "ads_shift_step");
	s_ads_shift_max						= pSettings->r_float("weapon_manager", "ads_shift_max");
	s_barrel_length_power				= pSettings->r_float("weapon_manager", "barrel_length_power");

	s_recoil_hud_stopping_power_per_shift = pSettings->r_float("weapon_manager", "recoil_hud_stopping_power_per_shift");
	s_recoil_hud_relax_impulse_per_shift = pSettings->r_float("weapon_manager", "recoil_hud_relax_impulse_per_shift");
	s_recoil_cam_angle_per_delta		= pSettings->r_float("weapon_manager", "recoil_cam_angle_per_delta");
	s_recoil_cam_stopping_power_per_impulse = pSettings->r_float("weapon_manager", "recoil_cam_stopping_power_per_impulse");
	s_recoil_cam_relax_impulse_ratio	= pSettings->r_float("weapon_manager", "recoil_cam_relax_impulse_ratio");
}
