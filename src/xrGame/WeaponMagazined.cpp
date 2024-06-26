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

	if (auto ao = cast<CAddonOwner*>())
	{
		for (auto& s : ao->AddonSlots())
		{
			if (s->attach == "magazine")
				m_magazine_slot			= s.get();
			if (s->attach == "muzzle")
				s->model_offset.translate_add(static_cast<Dvector>(m_fire_point));
		}
	}

	m_IronSightsZeroing.Load			(pSettings->r_string(section, "zeroing"));
	m_animation_slot_reloading			= READ_IF_EXISTS(pSettings, r_u32, section, "animation_slot_reloading", m_animation_slot);
	m_lock_state_reload					= !!READ_IF_EXISTS(pSettings, r_bool, section, "lock_state_reload", FALSE);
	m_mag_attach_bolt_release			= !!READ_IF_EXISTS(pSettings, r_bool, section, "mag_attach_bolt_release", FALSE);
	m_iron_sight_section				= READ_IF_EXISTS(pSettings, r_string, section, "iron_sight_section", 0);
	m_iron_sights						= (m_iron_sight_section.size()) ? 0 : 1;
	m_iron_sights_blockers				= m_iron_sights;

	LPCSTR integrated_addons			= READ_IF_EXISTS(pSettings, r_string, section, "integrated_addons", 0);
	for (int i = 0, cnt = _GetItemCount(integrated_addons); i < cnt; i++)
	{
		string64						addon_sect;
		_GetItem						(integrated_addons, i, addon_sect);
		CAddon::addAddonModules			(*this, addon_sect);
	}
	process_addon_modules				(*this, true);
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
	{
		bool lock						= (!m_locked && HudAnimationExist("anm_shot_l") && HudAnimationExist("anm_bolt_lock") && !get_cartridge_from_mag(m_cartridge, false));
		StartReload						((lock) ? eSubstateReloadBoltLock : eSubstateReloadBolt);
	}
	else if (!m_actor && has_ammo_for_reload())
		StartReload						(HudAnimationExist("anm_detach") ? eSubstateReloadDetach : eSubstateReloadBegin);
}

void CWeaponMagazined::StartReload(EWeaponSubStates substate)
{
	if (GetState() == eReload)
		return;
	
	if (m_lock_state_reload && !m_locked && isEmptyChamber() &&
		(m_magazine_slot->getLoadingAddon() && !m_magazine_slot->getLoadingAddon()->cast<CMagazine*>()->Empty() || !m_actor))
		m_sub_state						= eSubstateReloadBoltLock;
	else
		m_sub_state						= substate;

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

	// ����� �� ������
	StartFlameParticles();

	//��� �� ������
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

	m_sub_state = eSubstateReloadBegin;
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

	if (m_locked)
	{
		m_sub_state = eSubstateReloadBolt;
		PlayAnimReload();
	}
	else
	{
		m_sub_state = eSubstateReloadBegin;
		PlayAnimHide();
		if (m_sounds_enabled)
			PlaySound("sndHide", get_LastFP());
	}
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
	if (!m_usable)
		return false;

	if (m_hud->Action(cmd, flags))
		return true;

	if (inherited::Action(cmd, flags))
		return true;

	//���� ������ ���-�� ������, �� ������ �� ������
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

void CWeaponMagazined::updataeSilencerKoeffs()
{
	if (m_silencer)
	{
		m_silencer_koef.bullet_speed = pSettings->r_float(m_silencer->getSection(), "bullet_speed_k");
		m_silencer_koef.fire_dispersion = pSettings->r_float(m_silencer->getSection(), "fire_dispersion_base_k");
	}
	else
		m_silencer_koef.Reset();
}

//������������ ������� �������� ���������� � ���������
void CWeaponMagazined::on_firemode_switch()
{
	if (HudAnimationExist("anm_firemode"))
	{
		SwitchState(eFiremode);
		PlayHUDMotion("anm_firemode", TRUE, GetState());
	}
	if (m_sounds_enabled)
		PlaySound("sndFiremode", get_LastFP());
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
#include "ui\UIHudStatesWnd.h"
void CWeaponMagazined::GetBriefInfo(SWpnBriefInfo& info)
{
	VERIFY								(m_pInventory);

	CScope* scope						= getActiveScope();
	if (!scope)
		scope							= m_selected_scopes[0];
	if (!scope)
		scope							= m_selected_scopes[1];

	info.zeroing.printf					("%d %s", (scope) ? scope->Zeroing() : m_IronSightsZeroing.current, *CStringTable().translate("st_m"));
	if (scope && scope->Type() != CScope::eIS)
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

	CAddonOwner CPC ao = cast<CAddonOwner CP$>();
	if (ao)
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
		if (s->Type() == CScope::eOptics)
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
	CScope* scope						= getActiveScope();
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && scope)
		scope->ZoomChange				(1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange		(1);
		else if (ADS() == 1)
			m_IronSightsZeroing.Shift	(1);
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
	else
		inherited::ZoomInc				();
}

void CWeaponMagazined::ZoomDec()
{
	CScope* scope						= getActiveScope();
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && scope)
		scope->ZoomChange				(-1);
	else if (pInput->iGetAsyncKeyState(DIK_LALT))
	{
		if (scope)
			scope->ZeroingChange		(-1);
		else if (ADS() == 1)
			m_IronSightsZeroing.Shift	(-1);
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
	else
		inherited::ZoomDec				();
}

bool CWeaponMagazined::need_renderable()
{
	CScope* scope = getActiveScope();
	if (!scope || scope->Type() != CScope::eOptics || scope->isPiP())
		return true;
	return !IsRotatingToZoom();
}

bool CWeaponMagazined::render_item_ui_query()
{
	CScope* scope = getActiveScope();
	if (!scope || scope->Type() != CScope::eOptics)
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
	CScope* scope = getActiveScope();
	if (scope && scope->Type() == CScope::eOptics && (!scope->isPiP() && !IsRotatingToZoom() || !for_actor))
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

shared_str iron_sights_bone				= "iron_sights";
shared_str iron_sights_lowered_bone		= "iron_sights_lowered";
static void update_barrel_sights_visibility(CBarrel* barrel, bool status)
{
	barrel->O.UpdateBoneVisibility		(iron_sights_bone, status);
	barrel->O.UpdateBoneVisibility		(iron_sights_lowered_bone, !status);
	barrel->I->SetInvIconType			(!status);
}

void CWeaponMagazined::process_addon(CAddon* addon, bool attach)
{
	if (addon->getSlot()->blocking_iron_sights == 2 || (addon->getSlot()->blocking_iron_sights == 1 && !addon->isLowProfile()))
	{
		m_iron_sights_blockers			+= (attach) ? 1 : -1;
		if (m_iron_sight_section.size() && addon->O.cNameSect() == m_iron_sight_section)
			m_iron_sights				+= (attach) ? 1 : -1;
		bool status						= iron_sights_up();

		UpdateBoneVisibility			(iron_sights_bone, status);
		UpdateBoneVisibility			(iron_sights_lowered_bone, !status);
		UpdateHudBonesVisibility		(status);
		SetInvIconType					(!iron_sights_up());

		if (m_barrel && m_barrel->cast<CAddon*>())
			update_barrel_sights_visibility(m_barrel, status);
	}

	process_addon_modules				(addon->O, attach);
}

void CWeaponMagazined::process_addon_modules(CGameObject& obj, bool attach)
{
	if (auto mag = obj.Cast<CMagazine*>())
		m_magazine						= (attach) ? mag : nullptr;
	if (auto scope = obj.Cast<CScope*>())
		process_scope					(scope, attach);
	if (auto barrel = obj.Cast<CBarrel*>())
		process_barrel					(barrel, attach);
	if (auto muzzle = obj.Cast<CMuzzle*>())
		process_muzzle					(muzzle, attach);
	if (auto sil = obj.Cast<CSilencer*>())
		process_silencer				(sil, attach);
	
	shared_str type						= READ_IF_EXISTS(pSettings, r_string, obj.cNameSect(), "grip_type", 0);
	if (type.size())
	{
		m_grip							= attach;
		m_grip_accuracy_modifier		= (attach) ? readAccuracyModifier(*type) : 1.f;
	}
	
	type								= READ_IF_EXISTS(pSettings, r_string, obj.cNameSect(), "stock_type", 0);
	if (type.size())
	{
		m_stock_recoil_pattern			= (attach) ? readRecoilPattern(*type) : vOne;
		m_stock_accuracy_modifier		= (attach) ? readAccuracyModifier(*type) : 1.f;
	}
	
	type								= READ_IF_EXISTS(pSettings, r_string, obj.cNameSect(), "foregrip_type", 0);
	if (type.size())
	{
		if (!m_handguard.size())
			m_handguard					= type;
		if (m_handguard == type)
		{
			m_foregrip_recoil_pattern	= (attach) ? readRecoilPattern(*type) : vOne;
			m_foregrip_accuracy_modifier= (attach) ? readAccuracyModifier(*type) : 1.f;
			m_anm_prefix				= (attach) ? pSettings->r_string("anm_prefixes", *type) : 0;
			if (!attach)
				m_handguard				= 0;
		}
		else
		{
			m_foregrip_recoil_pattern	= readRecoilPattern(*((attach) ? type : m_handguard));
			m_foregrip_accuracy_modifier= readAccuracyModifier(*((attach) ? type : m_handguard));
			m_anm_prefix				= pSettings->r_string("anm_prefixes", *((attach) ? type : m_handguard));
		}
		PlayAnimIdle					();
	}
	
	m_usable							= m_barrel && m_grip;
	hud_sect							= pSettings->r_string(cNameSect(), (m_usable) ? "hud" : "hud_unusable");
}

void CWeaponMagazined::process_scope(CScope* scope, bool attach)
{
	m_hud->processScope					(scope, attach);

	if (attach)
	{
		m_attached_scopes.push_back		(scope);
		if (scope->getSelection() == -1)
		{
			for (int i = 0; i < 2; i++)
			{
				if (!m_selected_scopes[i] || m_selected_scopes[i]->Type() == CScope::eIS && scope->Type() != CScope::eIS)
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

void CWeaponMagazined::process_barrel(CBarrel* barrel, bool attach)
{
	if (attach)
	{
		m_barrel						= barrel;
		m_barrel_len					= pow(m_barrel->getLength(), s_barrel_length_power);
		load_muzzle_params				(m_barrel);
		if (m_barrel->cast<CAddon*>())
			update_barrel_sights_visibility(m_barrel, iron_sights_up());
	}
	else
	{
		m_barrel						= nullptr;
		m_barrel_len					= 0.f;
	}
}

void CWeaponMagazined::process_muzzle(CMuzzle* muzzle, bool attach)
{
	m_muzzle							= (attach) ? muzzle : nullptr;
	load_muzzle_params					((m_muzzle) ? m_muzzle : static_cast<CMuzzleBase*>(m_barrel));
}

void CWeaponMagazined::process_silencer(CSilencer* sil, bool attach)
{
	m_silencer							= (attach) ? sil : nullptr;
	UpdateSndShot						();
	updataeSilencerKoeffs				();
	load_muzzle_params					((attach) ? sil : ((m_muzzle) ? m_muzzle : static_cast<CMuzzleBase*>(m_barrel)));
}

void CWeaponMagazined::load_muzzle_params(CMuzzleBase* src)
{
	m_muzzle_recoil_pattern				= readRecoilPattern(*src->getSection(), "muzzle_type");
	m_fire_point						= src->getFirePoint();
	m_hud->calculateAimOffsets			();

	LoadLights							(*src->getSection());
	LoadFlameParticles					(*cNameSect());
	LoadFlameParticles					(*src->getSection());
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
		UpdateHudBonesVisibility		(iron_sights_up());
		break;
	case eUpdateSlotsTransform:
	{
		float res						= inherited::Aboba(type, data, param);
		if (auto scope = getActiveScope())
			if (scope->Type() == CScope::eOptics)
				scope->updateCameraLenseOffset();
		return							res;
	}
	case eSyncData:
	{
		float res						= inherited::Aboba(type, data, param);
		auto se_obj						= (CSE_ALifeDynamicObject*)data;
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
	if (m_usable)
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

void CWeaponMagazined::UpdateHudBonesVisibility(bool status)
{
	if (auto hi = HudItemData())
	{
		hi->set_bone_visible			(iron_sights_bone, status, TRUE);
		hi->set_bone_visible			(iron_sights_lowered_bone, !status, TRUE);
	}
}

void CWeaponMagazined::UpdateShadersDataAndSVP(CCameraManager& camera)
{
	CScope* scope						= getActiveScope();
	Device.SVP.setActive(scope && scope->isPiP());
	if (!scope || scope->Type() == CScope::eIS)
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
	if (scope->Type() == CScope::eOptics)
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
	CScope* active_scope				= getActiveScope();
	return								(active_scope) ? active_scope->Zeroing() : m_IronSightsZeroing.current;
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
	float speed							= m_barrel_len * m_silencer_koef.bullet_speed * c.param_s.bullet_speed_per_barrel_len * air_resistance_correction;

	Fvector								result[2];
	TransferenceAndThrowVelToThrowDir	(transference, speed, Level().BulletManager().GravityConst(), result);
	result[0].normalize					();

	return								result[0];
}

void CWeaponMagazined::SetADS(int mode)
{
	bool prev							= !!ADS();
	inherited::SetADS					(mode);
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

void CWeaponMagazined::loadStaticVariables()
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
