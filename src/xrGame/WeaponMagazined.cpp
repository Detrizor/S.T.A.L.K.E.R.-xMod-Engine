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

ENGINE_API	bool	g_dedicated_server;

CWeaponMagazined::CWeaponMagazined(ESoundTypes eSoundType) : CWeapon()
{
	m_eSoundShow = ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide = ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundSwitchMode = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	m_sounds_enabled = true;
	m_sSndShotCurrent = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_fOldBulletSpeed = 0.f;
	m_iQueueSize = -1;
	m_bHasDifferentFireModes = false;
	m_iCurFireMode = -1;
}

CWeaponMagazined::~CWeaponMagazined()
{
	xr_delete(m_hud);

	if (m_pSilencer && !m_pSilencer->cast<CAddon*>())
		xr_delete(m_pSilencer);
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
	m_sounds.LoadSound					(*HudSection(), "snd_switch_mode", "sndSwitchMode", true, m_eSoundSwitchMode);
	m_sounds.LoadSound					(*HudSection(), "snd_reload", "sndReload", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_detach", "sndDetach", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_attach", "sndAttach", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_bolt_pull", "sndBoltPull", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_bolt_release", "sndBoltRelease", true, m_eSoundReload);
	m_sounds.LoadSound					(*HudSection(), "snd_load_chamber", "sndLoadChamber", true, m_eSoundReload);

	m_iBaseDispersionedBulletsCount = READ_IF_EXISTS(pSettings, r_u8, section, "base_dispersioned_bullets_count", 0);
	m_fBaseDispersionedBulletsSpeed = READ_IF_EXISTS(pSettings, r_float, section, "base_dispersioned_bullets_speed", m_fStartBulletSpeed);

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
	InitRotateTime						();

	CAddonOwner* ao						= cast<CAddonOwner*>();
	if (ao)
	{
		for (auto s : ao->AddonSlots())
		{
			if (s->hasLoadingAnim())
				m_magazine_slot			= s;
			if (s->muzzle)
				s->model_offset.translate_add(m_loaded_muzzle_point);
			s->model_offset.translate_sub(m_root_bone_position);
		}
	}
	
	shared_str integrated_addon			= READ_IF_EXISTS(pSettings, r_string, section, "scope", "");
	if (integrated_addon.size())
	{
		CScope* scope					= xr_new<CScope>(this, integrated_addon);
		process_scope					(scope, true);
	}

	integrated_addon					= READ_IF_EXISTS(pSettings, r_string, section, "silencer", "");
	if (integrated_addon.size())
		ProcessSilencer					(xr_new<CSilencer>(this, integrated_addon), true);

	m_IronSightsZeroing.Load			(pSettings->r_string(section, "zeroing"));
	m_lower_iron_sights_on_block		= !!READ_IF_EXISTS(pSettings, r_bool, section, "lower_iron_sights_on_block", FALSE);
	m_animation_slot_reloading			= READ_IF_EXISTS(pSettings, r_u32, section, "animation_slot_reloading", m_animation_slot);
}

void CWeaponMagazined::FireStart()
{
	if (!IsMisfire())
	{
		if (hasAmmoToShoot())
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
	return								(unlimited_ammo()) ? true : (m_current_ammo && m_current_ammo->m_boxSize >= count);
}

void CWeaponMagazined::Reload()
{
	if (!m_pInventory && GetState() != eIdle)
	{
		SwitchState						(eIdle);
		return;
	}

	if (IsMisfire() || m_actor && m_chamber.capacity())
		StartReload						(eSubstateReloadBolt);
	else if (!m_actor && has_ammo_for_reload())
		StartReload						(eSubstateReloadBegin);
}

void CWeaponMagazined::StartReload(EWeaponSubStates substate)
{
	if (GetState() == eReload)
		return;

	m_sub_state							= substate;
	if (m_sub_state == eSubstateReloadBegin && HudAnimationExist("anm_detach"))
		m_sub_state						= eSubstateReloadDetach;

	SwitchState							(eReload);
}

bool CWeaponMagazined::Discharge(CCartridge& destination)
{
	if (m_actor)
	{
#ifdef	EXTENDED_WEAPON_CALLBACKS
		int	AC = GetSuitableAmmoTotal();
		m_actor->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
#endif
	}

	if (m_chamber.capacity())
	{
		if (m_chamber.size())
		{
			reload_chamber				(&destination);
			return						true;
		}
	}
	else if (m_magazin.capacity())
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
	if (chamber || !m_magazine_slot)
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
	if (GetState() == eIdle)
	{
		OnEmptyClick();
		return;
	}

	inherited::OnMagazineEmpty();
}

LPCSTR CWeaponMagazined::anmType() const
{
	if (GetState() == eReload && m_sub_state == eSubstateReloadBolt && !m_shot_shell && m_chamber.empty())
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
	if (m_chamber.capacity())
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
			CInventoryOwner* IO			= Parent->Cast<CInventoryOwner*>();
			IO->GiveAmmo				(*cartridge.m_ammoSect, 1, cartridge.m_fCondition);
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

	if (from_mag && !get_cartridge_from_mag(m_cartridge))
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

	if (GetCurrentFireMode() == -1 || m_iShotNum > m_iBaseDispersionedBulletsCount || !bWorking)
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
	m_sounds.SetPosition("sndLoadChamber", P);
}

void CWeaponMagazined::state_Fire(float dt)
{
	if (hasAmmoToShoot())
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

		while (hasAmmoToShoot() && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
		{
			if (CheckForMisfire())
			{
				StopShooting();
				return;
			}

			m_bFireSingleShot = false;

			//Alundaio: Use fModeShotTime instead of fOneShotTime if current fire mode is 2-shot burst
			//Alundaio: Cycle down RPM after two shots; used for Abakan/AN-94
			if (GetCurrentFireMode() == 2 || (bCycleDown == true && m_iShotNum < 1) )
				fShotTimeCounter = fModeShotTime;
			else
				fShotTimeCounter = fOneShotTime;
			//Alundaio: END

			++m_iShotNum;
			FireTrace();
			OnShot();
		}

		if (m_iShotNum == m_iQueueSize)
			m_bStopedAfterQueueFired = true;

		UpdateSounds();
	}

	if (fShotTimeCounter < 0)
	{
		if (!hasAmmoToShoot())
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
}

void CWeaponMagazined::switch2_Idle()
{
	m_iShotNum = 0;
	if (m_fOldBulletSpeed != 0.f)
		SetBulletSpeed(m_fOldBulletSpeed);

	SetPending(FALSE);
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
	OnZoomOut();
	CWeapon::FireEnd();
	
	SetPending(TRUE);
	PlayAnimHide();
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
	PlayAnimShow();
	if (m_sounds_enabled)
		PlaySound("sndShow", get_LastFP());
}

bool CWeaponMagazined::Action(u16 cmd, u32 flags)
{
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

void CWeaponMagazined::LoadSilencerKoeffs(LPCSTR sect)
{
	m_silencer_koef.bullet_speed = pSettings->r_float(sect, "bullet_speed_k");
	m_silencer_koef.fire_dispersion = pSettings->r_float(sect, "fire_dispersion_base_k");
}

void CWeaponMagazined::ResetSilencerKoeffs()
{
	m_silencer_koef.Reset();
}

void CWeaponMagazined::OnZoomIn()
{
	inherited::OnZoomIn();

	if (GetState() == eIdle)
		PlayAnimIdle();

	//Alundaio: callback not sure why vs2013 gives error, it's fine
#ifdef EXTENDED_WEAPON_CALLBACKS
	CGameObject	*object = smart_cast<CGameObject*>(H_Parent());
	if (object)
		object->callback(GameObject::eOnWeaponZoomIn)(object->lua_game_object(),this->lua_game_object());
#endif
	//-Alundaio

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor)
	{
		CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(pActor->Cameras().GetCamEffector(eCEZoom));
		if (!S)
		{
			S = (CEffectorZoomInertion*) pActor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion>());
			S->Init(this);
		}
		S->SetRndSeed(pActor->GetZoomRndSeed());
		R_ASSERT(S);
	}
}

void CWeaponMagazined::OnZoomOut()
{
	if (!IsZoomed())
		return;

	inherited::OnZoomOut();

	if (GetState() == eIdle)
		PlayAnimIdle();

	//Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
	CGameObject	*object = smart_cast<CGameObject*>(H_Parent());
	if (object)
		object->callback(GameObject::eOnWeaponZoomOut)(object->lua_game_object(), this->lua_game_object());
#endif
	//-Alundaio

	CActor* pActor = smart_cast<CActor*>(H_Parent());

	if (pActor)
		pActor->Cameras().RemoveCamEffector(eCEZoom);
}

//переключение режимов стрельбы одиночными и очередями
void CWeaponMagazined::on_firemode_switch()
{
	if (HudAnimationExist("anm_switch_firemode"))
		PlayHUDMotion("anm_switch_firemode", TRUE, GetState());
	PlaySound("sndSwitchMode", get_LastFP());
}

void CWeaponMagazined::OnNextFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
	on_firemode_switch();
}

void CWeaponMagazined::OnPrevFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
	on_firemode_switch();
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
bool CWeaponMagazined::GetBriefInfo(II_BriefInfo& info)
{
	VERIFY(m_pInventory);
	string32	int_str;

	CScope* scope						= getActiveScope();
	if (!scope)
		scope							= m_selected_scopes[0];
	if (!scope)
		scope							= m_selected_scopes[1];
	info.cur_ammo.printf				("%d %s", (scope) ? scope->Zeroing() : m_IronSightsZeroing.current, *CStringTable().translate("st_m"));
	if (scope && scope->Type() == eOptics)
	{
		float magnification				= scope->GetCurrentMagnification();
		info.fmj_ammo.printf			(fEqual(round(magnification), magnification) ? "%.0fx" : "%.1fx", magnification);

	}
	else
		info.fmj_ammo._set				("");

	if (HasFireModes())
	{
		if (m_iQueueSize < 0)
			info.fire_mode = "A";
		else
		{
			xr_sprintf(int_str, "%d", m_iQueueSize);
			info.fire_mode = int_str;
		}
	}
	else
		info.fire_mode = "";

	if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
	{
		return false;
	}
	GetSuitableAmmoTotal();//update m_BriefInfo_CalcFrame
	info.grenade = "";

	u32 at_size = m_ammoTypes.size();
	if (unlimited_ammo() || at_size == 0)
	{
		info.ap_ammo._set("--");
		info.third_ammo._set("--"); //Alundaio
	}
	else
	{
		//Alundaio: Added third ammo type and cleanup
		info.ap_ammo._set("");
		info.third_ammo._set("");

		if (at_size >= 1)
		{
			xr_sprintf(int_str, "%d", GetAmmoCount(0));
			info.fmj_ammo._set(int_str);
		}
		if (at_size >= 2)
		{
			xr_sprintf(int_str, "%d", GetAmmoCount(1));
			info.ap_ammo._set(int_str);
		}
		if (at_size >= 3)
		{
			xr_sprintf(int_str, "%d", GetAmmoCount(2));
			info.third_ammo._set(int_str);
		}
		//-Alundaio
	}

	info.icon = (m_chamber.size()) ? m_chamber.back().m_ammoSect : "";
	return true;
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
	result |= process_if_exists(section, "base_dispersioned_bullets_speed", m_fBaseDispersionedBulletsSpeed, test);

	return result;
}
//текущая дисперсия (в радианах) оружия с учетом используемого патрона и недисперсионных пуль
float CWeaponMagazined::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
	float fire_disp = GetBaseDispersion(cartridge_k);
	if (for_crosshair || !m_iBaseDispersionedBulletsCount || !m_iShotNum || m_iShotNum > m_iBaseDispersionedBulletsCount)
	{
		fire_disp = inherited::GetFireDispersion(cartridge_k);
	}
	return fire_disp;
}
void CWeaponMagazined::FireBullet(const Fvector& pos,
	const Fvector& shot_dir,
	float fire_disp,
	const CCartridge& cartridge,
	u16 parent_id,
	u16 weapon_id,
	bool send_hit)
{
	if (m_iBaseDispersionedBulletsCount)
	{
		if (m_iShotNum <= 1)
		{
			m_fOldBulletSpeed = GetBulletSpeed();
			SetBulletSpeed(m_fBaseDispersionedBulletsSpeed);
		}
		else if (m_iShotNum > m_iBaseDispersionedBulletsCount)
		{
			SetBulletSpeed(m_fOldBulletSpeed);
		}
	}

	inherited::FireBullet(pos, shot_dir, fire_disp, cartridge, parent_id, weapon_id, send_hit);
}

bool CWeaponMagazined::CanTrade() const
{
	if (GetAmmoElapsed())
		return false;

	CAddonOwner CPC ao = cast<CAddonOwner CP$>();
	if (ao)
	{
		for (auto slot : ao->AddonSlots())
		{
			if (!slot->addons.empty())
				return false;
		}
	}

	return true;
}

void UpdateBoneVisibility(IKinematics* pVisual, const shared_str& bone_name, bool status)
{
	u16 bone_id = pVisual->LL_BoneID(bone_name);
	if (bone_id != BI_NONE)
	{
		if (status)
		{
			if (!pVisual->LL_GetBoneVisible(bone_id))
				pVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else if (pVisual->LL_GetBoneVisible(bone_id))
			pVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	}
}

shared_str iron_sights_bone = "iron_sights";
shared_str iron_sights_lowered_bone = "iron_sights_lowered";

void CWeaponMagazined::UpdateBonesVisibility()
{
	IKinematics* pWeaponVisual			= smart_cast<IKinematics*>(Visual());
	R_ASSERT							(pWeaponVisual);

	pWeaponVisual->CalculateBones_Invalidate();
	UpdateBoneVisibility				(pWeaponVisual, iron_sights_bone, !m_iron_sights_blockers);
	UpdateBoneVisibility				(pWeaponVisual, iron_sights_lowered_bone, !!m_iron_sights_blockers);
	pWeaponVisual->CalculateBones_Invalidate();
	pWeaponVisual->CalculateBones		(TRUE);

	UpdateHudBonesVisibility			();
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

bool CWeaponMagazined::hasAmmoToShoot() const
{
	return (m_chamber.size() || (!m_chamber.capacity() && GetAmmoElapsed()));
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
		if (s->Type() == eOptics)
		{
			s->modify_holder_params		(range, fov);
			return;
		}
	}
}

CScope* CWeaponMagazined::getActiveScope() const
{
	if (ADS() == 1)
		return m_selected_scopes[0];
	else if (ADS() == -1)
		return m_selected_scopes[1];
	return NULL;
}

void CWeaponMagazined::ZoomInc()
{
	CScope* scope = getActiveScope();
	if (scope && pInput->iGetAsyncKeyState(DIK_LSHIFT))
		scope->ZoomChange(1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange(1);
		else
			m_IronSightsZeroing.Shift(1);
	}
	else if (ADS() && pInput->iGetAsyncKeyState(DIK_LCONTROL))
		cycle_scope(int(ADS() == -1), true);
	else
		inherited::ZoomInc();
}

void CWeaponMagazined::ZoomDec()
{
	CScope* scope = getActiveScope();
	if (scope && pInput->iGetAsyncKeyState(DIK_LSHIFT))
		scope->ZoomChange(-1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange(-1);
		else
			m_IronSightsZeroing.Shift(-1);
	}
	else if (ADS() && pInput->iGetAsyncKeyState(DIK_LCONTROL))
		cycle_scope(int(ADS() == -1), false);
	else
		inherited::ZoomDec();
}

bool CWeaponMagazined::need_renderable()
{
	CScope* scope = getActiveScope();
	if (!scope || scope->Type() != eOptics || scope->isPiP())
		return true;
	return !IsRotatingToZoom();
}

bool CWeaponMagazined::render_item_ui_query()
{
	CScope* scope = getActiveScope();
	if (!scope || scope->Type() != eOptics)
		return false;

	if (scope->isPiP())
		return (::Render->currentViewPort == SECONDARY_WEAPON_SCOPE);

	return !IsRotatingToZoom();
}

void CWeaponMagazined::render_item_ui()
{
	getActiveScope()->RenderUI();
}

float CWeaponMagazined::CurrentZoomFactor(bool for_actor) const
{
	CScope* scope = getActiveScope();
	if (scope && scope->Type() == eOptics && (!scope->isPiP() && !IsRotatingToZoom() || !for_actor))
		return scope->GetCurrentMagnification();
	else
		return inherited::CurrentZoomFactor(for_actor);
}

void CWeaponMagazined::ProcessMagazine(CMagazine* mag, bool attach)
{
	m_magazine							= (attach) ? mag : NULL;
}

void CWeaponMagazined::ProcessSilencer(CSilencer* sil, bool attach)
{
	m_pSilencer							= (attach) ? sil : NULL;

	shared_str CR$ sect_to_load			= ((attach) ? sil->Section() : cNameSect());
	LoadLights							(*sect_to_load);
	LoadFlameParticles					(*sect_to_load);
	if (attach)
		LoadSilencerKoeffs				(*sect_to_load);
	else
		ResetSilencerKoeffs				();
	UpdateSndShot						();

	if (sil->cast<CAddon*>())
		m_muzzle_point					= (attach) ? sil->getMuzzlePoint() : m_loaded_muzzle_point;
}

void CWeaponMagazined::process_scope(CScope* scope, bool attach)
{
	m_hud->ProcessScope					(scope, attach);
	if (attach)
	{
		m_attached_scopes.push_back		(scope);
		if (scope->getSelection() == -1)
		{
			for (int i = 0; i < 2; i++)
			{
				if (!m_selected_scopes[i] || m_selected_scopes[i]->Type() == eIS && scope->Type() != eIS)
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
}

void CWeaponMagazined::cycle_scope(int idx, bool up)
{
	if (m_attached_scopes.empty())
		return;

	CScope* cur_scope					= m_selected_scopes[idx];
	if (cur_scope)
	{
		cur_scope->setSelection			(-1);
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

void CWeaponMagazined::process_addon(CAddon* addon, bool attach)
{
	if (auto mag = addon->cast<CMagazine*>())
		ProcessMagazine			(mag, attach);

	if (auto scope = addon->cast<CScope*>())
		process_scope			(scope, attach);

	if (auto sil = addon->cast<CSilencer*>())
		ProcessSilencer			(sil, attach);

	InitRotateTime				();

	if (addon->anmPrefix().size())
	{
		m_anm_prefix			= (attach) ? addon->anmPrefix() : 0;
		PlayAnimIdle			();
	}

	if (pSettings->line_exist(addon->O.cNameSect(), "grip"))
		m_grip_accuracy_modifier = readAccuracyModifier((attach) ? *addon->O.cNameSect() : *m_section_id, "grip");
			
	if (addon->getSlot()->blocking_iron_sights == 2 || (addon->getSlot()->blocking_iron_sights == 1 && !addon->isLowProfile()))
	{
		m_iron_sights_blockers	+= (attach) ? 1 : -1;
		UpdateBonesVisibility	();
		if (m_lower_iron_sights_on_block)
			SetInvIconType		(!!m_iron_sights_blockers);
	}

	if (auto addon_ao = addon->cast<CAddonOwner*>())
		for (auto s : addon_ao->AddonSlots())
			for (auto a : s->addons)
				process_addon	(a, attach);
}

float CWeaponMagazined::Aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eOnAddon:
	{
		process_addon					((CAddon*)data, !!param);
		break;
	}
	case eWeight:
		return							inherited::Aboba(type, data, param) + GetMagazineWeight();
	case eTransferAddon:
	{
		auto addon						= (CAddon*)data;
		if (addon->cast<CMagazine*>())
		{
			m_magazine_slot->startReloading((param) ? addon : nullptr);
			StartReload					((m_magazine_slot->addons.empty()) ? eSubstateReloadAttach : eSubstateReloadDetach);
			return						1.f;
		}
		break;
	}
	case eUpdateHudBonesVisibility:
		UpdateHudBonesVisibility		();
		break;
	case eUpdateSlotsTransform:
	{
		float res						= inherited::Aboba(type, data, param);
		if (auto scope = getActiveScope())
			if (scope->Type() == eOptics)
				scope->updateCameraLenseOffset();
		return							res;
	}
	case eSyncData:
	{
		float res						= inherited::Aboba(type, data, param);
		auto se_obj						= (CSE_ALifeDynamicObject*)data;
		auto se_wpn						= smart_cast<CSE_ALifeItemWeaponMagazined*>(se_obj);
		if (param)
			se_wpn->m_u8CurFireMode		= (u8)m_iCurFireMode;
		else
		{
			m_iCurFireMode				= se_wpn->m_u8CurFireMode;
			SetQueueSize				(GetCurrentFireMode());
		}
		return							res;
	}
	}

	return								inherited::Aboba(type, data, param);
}

void CWeaponMagazined::UpdateSndShot()
{
	if (m_actor)
		m_sSndShotCurrent = (m_pSilencer) ? "sndSilencerShotActor" : "sndShotActor";
	else
		m_sSndShotCurrent = (m_pSilencer) ? "sndSilencerShot" : "sndShot";
}

void CWeaponMagazined::OnTaken()
{
	UpdateSndShot();
}

void CWeaponMagazined::UpdateHudAdditional(Fmatrix& trans)
{
	m_hud->UpdateHudAdditional(trans);
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

void CWeaponMagazined::InitRotateTime()
{
	m_hud->InitRotateTime(GetControlInertionFactor());
}

void CWeaponMagazined::UpdateHudBonesVisibility()
{
	attachable_hud_item* hi				= HudItemData();
	if (!hi)
		return;

	hi->set_bone_visible				(iron_sights_bone, !m_iron_sights_blockers, TRUE);
	hi->set_bone_visible				(iron_sights_lowered_bone, !!m_iron_sights_blockers, TRUE);
}

extern float aim_fov_tan;
void CWeaponMagazined::UpdateShadersDataAndSVP(CCameraManager& camera)
{
	CScope* scope						= getActiveScope();
	Device.m_SecondViewport.SetSVPActive(scope && scope->isPiP());
	if (!scope || (scope->Type() == eOptics && !scope->isPiP()))
		return;

	float								fov_tan;
	Fvector4& hud_params				= g_pGamePersistent->m_pGShaderConstants->hud_params;
	if (scope->Type() == eOptics)
	{
		float lense_fov_tan				= scope->getLenseFovTan();
		hud_params.w					= CurrentZoomFactor(false);
		hud_params.z					= lense_fov_tan / aim_fov_tan;
		fov_tan							= lense_fov_tan / hud_params.w;
		Device.m_SecondViewport.setFov	(atanf(fov_tan) / (.5f * PI / 180.f));
		
		Fvector pos						= scope->getSightPosition();
		pos.add							(scope->getObjectiveOffset());
		pos.z							+= EPS_L;

		Fmatrix CP$						trans;
		if (auto addon = scope->cast<CAddon*>())
			trans						= &addon->getHudTransform();
		else
			trans						= &HudItemData()->m_transform;
		trans->transform_tiny			(pos);

		Device.m_SecondViewport.setPosition(pos);
	}
	else
	{
		fov_tan							= aim_fov_tan;
		hud_params.z					= scope->GetReticleScale();
	}
	
	Fvector cam_dir						= camera.Direction();
	float cam_dir_yaw					= atan2f(cam_dir.x, cam_dir.z);
	float cam_dir_pitch					= asinf(cam_dir.y);
	
	Fvector fire_dir					= get_LastFD();
	float fire_dir_yaw					= atan2f(fire_dir.x, fire_dir.z);
	float fire_dir_pitch				= asinf(fire_dir.y);
	
	float distance						= scope->Zeroing();
	float x_derivation					= distance * tanf(fire_dir_yaw - cam_dir_yaw);
	float y_derivation					= distance * tanf(fire_dir_pitch - cam_dir_pitch);

	float h								= 2.f * fov_tan * distance;
	hud_params.x						= x_derivation / h;
	hud_params.y						= y_derivation / h;
}

u16 CWeaponMagazined::Zeroing C$()
{
	CScope* active_scope				= getActiveScope();
	return								(active_scope) ? active_scope->Zeroing() : m_IronSightsZeroing.current;
}

Fvector CWeaponMagazined::getFullFireDirection(CCartridge CR$ c)
{
	auto hi								= HudItemData();
	if (!hi)
		return							get_LastFD();

	float distance						= Zeroing();
	Fvector transference				= m_hud->getMuzzleSightOffset().mad(vForward, distance);
	hi->m_transform.transform_dir		(transference);
	float air_resistance_correction		= Level().BulletManager().CalcZeroingCorrection(c.param_s.fAirResistZeroingCorrection, distance);
	float speed							= m_fStartBulletSpeed * m_silencer_koef.bullet_speed * c.param_s.kBulletSpeed * air_resistance_correction;

	Fvector								result[2];
	TransferenceAndThrowVelToThrowDir	(transference, speed, Level().BulletManager().GravityConst(), result);
	result[0].normalize					();
	return								result[0];
}

#define s_recoil_hud_stopping_power_per_shift pSettings->r_float("weapon_manager", "recoil_hud_stopping_power_per_shift")
#define s_recoil_hud_relax_impulse_magnitude pSettings->r_float("weapon_manager", "recoil_hud_relax_impulse_magnitude")
#define s_recoil_cam_stopping_power_per_impulse pSettings->r_float("weapon_manager", "recoil_cam_stopping_power_per_impulse")
#define s_recoil_cam_relax_impulse_ratio pSettings->r_float("weapon_manager", "recoil_cam_relax_impulse_ratio")
void CWeaponMagazined::updateRecoil()
{
	bool hud_impulse = !fIsZero(m_recoil_hud_impulse.magnitude());
	bool hud_shift = !fIsZero(m_recoil_hud_shift.magnitude());
	bool cam_impulse = !fIsZero(m_recoil_cam_impulse.magnitude());
	if (!hud_impulse && !hud_shift && !cam_impulse)
		return;

	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);
	CEntityAlive* ea = smart_cast<CEntityAlive*>(H_Parent());
	float accuracy = (ea) ? ea->getAccuracy() : 1.f;
	accuracy *= m_grip_accuracy_modifier * m_stock_accuracy_modifier * m_layout_accuracy_modifier;
	float cif = GetControlInertionFactor();

	if (hud_impulse || hud_shift)
	{
		auto update_hud_recoil_shift = [this, cif](Fvector CR$ impulse)
			{
				Fvector delta = impulse;
				delta.mul(fAvgTimeDelta);
				delta.div(cif);
				m_recoil_hud_shift.add(delta);
			};

		if (hud_impulse)
		{
			update_hud_recoil_shift(m_recoil_hud_impulse);

			Fvector stopping_power = m_recoil_hud_impulse;
			stopping_power.normalize();
			stopping_power.mul(m_recoil_hud_shift.magnitude());
			stopping_power.mul(-s_recoil_hud_stopping_power_per_shift);
			stopping_power.mul(accuracy);
			stopping_power.mul(fAvgTimeDelta);
			m_recoil_hud_impulse.add(stopping_power);
			if (fMoreOrEqual(m_recoil_hud_impulse.dotproduct(stopping_power), 0.f))
				m_recoil_hud_impulse = vZero;
		}
		else
		{
			Fvector relax_impulse = m_recoil_hud_shift;
			relax_impulse.normalize();
			relax_impulse.mul(-s_recoil_hud_relax_impulse_magnitude);
			relax_impulse.mul(accuracy);
			update_hud_recoil_shift(relax_impulse);
			if (fMoreOrEqual(m_recoil_hud_shift.dotproduct(relax_impulse), 0.f))
				m_recoil_hud_shift = vZero;
		}
	}

	if (cam_impulse)
	{
		auto update_cam_recoil_delta = [this, cif](Fvector CR$ impulse)
			{
				m_recoil_cam_delta = impulse;
				m_recoil_cam_delta.mul(fAvgTimeDelta);
				m_recoil_cam_delta.div(cif);
			};

		if (cam_impulse)
		{
			update_cam_recoil_delta(m_recoil_cam_impulse);

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
}
