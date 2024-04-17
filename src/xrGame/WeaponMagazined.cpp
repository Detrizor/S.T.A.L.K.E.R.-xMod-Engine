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
	m_eSoundShot = ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundEmptyClick = ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundReload = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#ifdef NEW_SOUNDS
	m_eSoundReloadEmpty = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
#endif
	m_sounds_enabled = true;

	m_sSndShotCurrent = NULL;

	m_pCurrentAmmo = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_fOldBulletSpeed = 0.f;
	m_iQueueSize = WEAPON_ININITE_QUEUE;
	m_bLockType = false;
	m_bHasDifferentFireModes = false;
	m_iCurFireMode = -1;
	m_iPrefferedFireMode = -1;

	m_Chamber					= FALSE;
	m_pNextMagazine				= NULL;
	m_pCartridgeToReload		= NULL;

	m_pSilencer					= NULL;
	m_pMagazine					= NULL;
	m_pMagazineSlot				= NULL;

	m_ReloadHalfPoint			= 0.f;
	m_ReloadEmptyHalfPoint		= 0.f;
	m_ReloadPartialPoint		= 0.f;
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

//AVO: for custom added sounds check if sound exists
bool CWeaponMagazined::WeaponSoundExist(LPCSTR section, LPCSTR sound_name)
{
	LPCSTR str;
	bool sec_exist = process_if_exists(section, sound_name, str, true);
	if (sec_exist)
		return true;
	else
	{
#ifdef DEBUG
		Msg("~ [WARNING] ------ Sound [%s] does not exist in [%s]", sound_name, section);
#endif
		return false;
	}
}
//-AVO

void CWeaponMagazined::Load(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	m_sounds.LoadSound(section, "snd_draw", "sndShow", true, m_eSoundShow);
	m_sounds.LoadSound(section, "snd_holster", "sndHide", true, m_eSoundHide);

	shared_str tmp = "snd_shoot";
	m_layered_sounds.LoadSound(section, *tmp, "sndShot", false, m_eSoundShot);
	m_layered_sounds.LoadSound(section, (WeaponSoundExist(section, "snd_shoot_actor")) ? "snd_shoot_actor" : *tmp, "sndShotActor", false, m_eSoundShot);
	if (WeaponSoundExist(section, "snd_silncer_shot"))
		tmp = "snd_silncer_shot";
	m_layered_sounds.LoadSound(section, *tmp, "sndSilencerShot", false, m_eSoundShot);
	if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
		tmp = "snd_silncer_shot_actor";
	m_layered_sounds.LoadSound(section, *tmp, "sndSilencerShotActor", false, m_eSoundShot);

	m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", true, m_eSoundEmptyClick);
	m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);

	if (WeaponSoundExist(section, "snd_reload_empty"))
		m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
	if (WeaponSoundExist(section, "snd_reload_misfire"))
		m_sounds.LoadSound(section, "snd_reload_misfire", "sndReloadMisfire", true, m_eSoundReloadMisfire);

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
		m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
	}
	else
		m_bHasDifferentFireModes = false;

	m_Chamber = pSettings->r_bool(section, "has_chamber");

	m_hud								= xr_new<CWeaponHud>(this);
	InitRotateTime						();

	CAddonOwner* ao						= cast<CAddonOwner*>();
	if (ao)
	{
		for (auto s : ao->AddonSlots())
		{
			if (s->magazine)
				m_pMagazineSlot			= s;
			if (s->muzzle)
				s->model_offset.translate_add(m_loaded_muzzle_point);
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

	m_ReloadHalfPoint					= READ_IF_EXISTS(pSettings, r_float, hud_sect, "reload_half_point", .4f);
	m_ReloadEmptyHalfPoint				= READ_IF_EXISTS(pSettings, r_float, hud_sect, "reload_empty_half_point", .3f);
	m_ReloadPartialPoint				= READ_IF_EXISTS(pSettings, r_float, hud_sect, "reload_partial_point", .75f);

	m_IronSightsZeroing.Load			(pSettings->r_string(section, "zeroing"));

	m_lower_iron_sights_on_block		= !!READ_IF_EXISTS(pSettings, r_bool, section, "lower_iron_sights_on_block", FALSE);
}

void CWeaponMagazined::FireStart()
{
	if (!IsMisfire())
	{
		if (IsValid())
		{
			if (!IsWorking() || AllowFireWhileWorking())
			{
				if (GetState() == eReload) return;
				if (GetState() == eShowing) return;
				if (GetState() == eHiding) return;
				if (GetState() == eMisfire) return;

				inherited::FireStart();

				if (iAmmoElapsed == 0)
					OnMagazineEmpty();
				else
				{
					R_ASSERT(H_Parent());
					SwitchState(eFire);
				}
			}
		}
		else
		{
			if (eReload != GetState())
				OnMagazineEmpty();
		}
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

void CWeaponMagazined::FireEnd()
{
	inherited::FireEnd();
}

void CWeaponMagazined::Reload()
{
	if (!m_pInventory && GetState() != eIdle)
	{
		SwitchState(eIdle);
		return;
	}

	if (ParentIsActor())
	{
		if (iAmmoElapsed)
		{
			if (m_magazine.size())
			{
				CInventoryOwner* IO			= smart_cast<CInventoryOwner*>(Parent);
				CCartridge& cartridge		= m_magazine.back();
				IO->GiveAmmo				(*cartridge.m_ammoSect, 1, cartridge.m_fCondition);
				ConsumeShotCartridge		();
				m_pInventory->Ruck			(this);
				if (IsMisfire())
					bMisfire				= false;
			}
			else if (IsMisfire())
			{
				m_pInventory->Ruck			(this);
				bMisfire					= false;
			}
		}
	}
	else
	{
		m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));

		if (IsMisfire() && iAmmoElapsed)
		{
			StartReload();
			return;
		}

		if (m_pCurrentAmmo || unlimited_ammo())
		{
			StartReload();
			return;
		}
		else for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
		{
			m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
			if (m_pCurrentAmmo)
			{
				m_set_next_ammoType_on_reload = i;
				StartReload();
				return;
			}
		}
	}
}

void CWeaponMagazined::StartReload(CObject* to_reload)
{
	CWeapon::Reload						();
	SetPending							(TRUE);
	SwitchState							(eReload);
	m_pNextMagazine						= cast<CMagazine*>(to_reload);
	m_pCartridgeToReload				= cast<CWeaponAmmo*>(to_reload);
}

bool CWeaponMagazined::IsAmmoAvailable()
{
	if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str())))
		return	(true);
	else
		for (u32 i = 0; i < m_ammoTypes.size(); ++i)
			if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str())))
				return	(true);
	return		(false);
}

bool CWeaponMagazined::Discharge(CCartridge& destination)
{
	if (m_magazine.empty())
		return false;
	if (ParentIsActor())
	{
#ifdef	EXTENDED_WEAPON_CALLBACKS
		int	AC = GetSuitableAmmoTotal();
		Actor()->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
#endif
	}
	CCartridge& back_cartridge	= m_magazine.back();
	destination.m_ammoSect		= back_cartridge.m_ammoSect;
	destination.m_fCondition	= back_cartridge.m_fCondition;
	m_magazine.pop_back			();
	--iAmmoElapsed;
	return						true;
}

void CWeaponMagazined::OnMagazineEmpty()
{
#ifdef	EXTENDED_WEAPON_CALLBACKS
	if (ParentIsActor())
	{
		int	AC = GetSuitableAmmoTotal();
		Actor()->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
	}
#endif
	if (GetState() == eIdle)
	{
		OnEmptyClick();
		return;
	}

	if (GetNextState() != eMagEmpty && GetNextState() != eReload)
	{
		SwitchState(eMagEmpty);
	}

	inherited::OnMagazineEmpty();
}

void CWeaponMagazined::PrepareCartridgeToShoot()
{
	if (m_magazine.empty())
		LoadCartridgeFromMagazine();
}

void CWeaponMagazined::LoadCartridgeFromMagazine(bool set_ammo_type_only)
{
	if (!m_pMagazine || m_pMagazine->Empty())
		return;

	CCartridge						cartridge;
	m_pMagazine->GetCartridge		(cartridge, !set_ammo_type_only);
	FindAmmoClass					(*cartridge.m_ammoSect, true);
	if (!set_ammo_type_only)
	{
		cartridge.m_LocalAmmoType	= m_ammoType;
		m_magazine.push_back		(cartridge);
	}
}

void CWeaponMagazined::ConsumeShotCartridge()
{
	inherited::ConsumeShotCartridge();
	if (m_magazine.empty())
		LoadCartridgeFromMagazine(!Chamber());
}

bool CWeaponMagazined::LoadCartridge(CWeaponAmmo* cartridges)
{
	if (FindAmmoClass(*cartridges->m_section_id))
	{
		if (Chamber())
		{
			if (!iAmmoElapsed)
			{
				CCartridge				l_cartridge;
				cartridges->Get			(l_cartridge);
				l_cartridge.m_LocalAmmoType = m_ammoType;
				m_magazine.push_back	(l_cartridge);
				++iAmmoElapsed;
				return					true;
			}
		}
		else if (iAmmoElapsed < iMagazineSize)
		{
			StartReload					(cartridges);
			return						true;
		}
	}
	return								false;
}

void CWeaponMagazined::ReloadMagazine()
{
	if (ParentIsActor())
	{
		if (m_pNextMagazine)
			m_pNextMagazine->Transfer(ID());
		else if (m_pCartridgeToReload)
		{
			FindAmmoClass				(*m_pCartridgeToReload->m_section_id, true);
			CCartridge					l_cartridge;
			while (iAmmoElapsed < iMagazineSize)
			{
				if (!m_pCartridgeToReload->Get(l_cartridge))
					break;
				l_cartridge.m_LocalAmmoType	= m_ammoType;
				m_magazine.push_back	(l_cartridge);
				++iAmmoElapsed;
			}
		}
	}
	else
	{
		m_BriefInfo_CalcFrame = 0;

		//устранить осечку при перезарядке
		if (IsMisfire())	bMisfire = false;

		if (!m_bLockType)
		{
			m_pCurrentAmmo = NULL;
		}

		if (!m_pInventory) return;

		if (m_set_next_ammoType_on_reload != undefined_ammo_type)
		{
			m_ammoType = m_set_next_ammoType_on_reload;
			m_set_next_ammoType_on_reload = undefined_ammo_type;
		}

		if (!unlimited_ammo())
		{
			if (m_ammoTypes.size() <= m_ammoType)
				return;

			LPCSTR tmp_sect_name = m_ammoTypes[m_ammoType].c_str();

			if (!tmp_sect_name)
				return;

			//попытаться найти в инвентаре патроны текущего типа
			m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(tmp_sect_name));

			if (!m_pCurrentAmmo && !m_bLockType)
			{
				for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
				{
					//проверить патроны всех подходящих типов
					m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
					if (m_pCurrentAmmo)
					{
						m_ammoType = i;
						break;
					}
				}
			}
		}

		//нет патронов для перезарядки
		if (!m_pCurrentAmmo && !unlimited_ammo())
			return;

		VERIFY((u32)iAmmoElapsed == m_magazine.size());

		if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
			m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
		CCartridge l_cartridge = m_DefaultCartridge;
		while (iAmmoElapsed < iMagazineSize)
		{
			if (!unlimited_ammo())
			{
				if (!m_pCurrentAmmo->Get(l_cartridge)) break;
			}
			++iAmmoElapsed;
			l_cartridge.m_LocalAmmoType = m_ammoType;
			m_magazine.push_back(l_cartridge);
		}

		VERIFY((u32)iAmmoElapsed == m_magazine.size());

		//выкинуть коробку патронов, если она пустая
		if (m_pCurrentAmmo && !m_pCurrentAmmo->GetAmmoCount() && OnServer())
			m_pCurrentAmmo->SetDropManual(TRUE);

		if (iMagazineSize > iAmmoElapsed)
		{
			m_bLockType = true;
			ReloadMagazine();
			m_bLockType = false;
		}

		VERIFY((u32)iAmmoElapsed == m_magazine.size());
	}
}

xr_vector<CCartridge>& CWeaponMagazined::Magazine()
{
	return m_magazine;
}

u32 CWeaponMagazined::MagazineElapsed()
{
	return Magazine().size();
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
	case eMagEmpty:
		switch2_Empty();
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

	//когда происходит апдейт состояния оружия
	//ничего другого не делать
	if (GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eShowing:
		case eHiding:
		case eReload:
		case eIdle:
		{
			fShotTimeCounter -= dt;
			clamp(fShotTimeCounter, 0.0f, flt_max);
		}break;
		case eFire:
		{
			state_Fire(dt);
		}break;
		case eMisfire:		state_Misfire(dt);	break;
		case eMagEmpty:		state_MagEmpty(dt);	break;
		case eHidden:		break;
		}
	}

	UpdateSounds();

	if (GetCurrentFireMode() == -1 || m_iShotNum > m_iBaseDispersionedBulletsCount || !bWorking)
		updateRecoil();
}

void CWeaponMagazined::UpdateSounds()
{
	if (Device.dwFrame == dwUpdateSounds_Frame)
		return;

	dwUpdateSounds_Frame = Device.dwFrame;

	Fvector P = get_LastFP();
	m_sounds.SetPosition("sndShow", P);
	m_sounds.SetPosition("sndHide", P);
	//. nah	m_sounds.SetPosition("sndShot", P);
	m_sounds.SetPosition("sndReload", P);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
	if (m_sounds.FindSoundItem("sndReloadEmpty", false))
		m_sounds.SetPosition("sndReloadEmpty", P);
#endif //-NEW_SOUNDS

	//. nah	m_sounds.SetPosition("sndEmptyClick", P);
}

void CWeaponMagazined::state_Fire(float dt)
{
	if (iAmmoElapsed > 0)
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

		while (iAmmoElapsed && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
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
			{
				fShotTimeCounter = fModeShotTime;
			}
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
		/*
				if(bDebug && H_Parent() && (H_Parent()->ID() != Actor()->ID()))
				{
				Msg("stop shooting w=[%s] magsize=[%d] sshot=[%s] qsize=[%d] shotnum=[%d]",
				IsWorking()?"true":"false",
				m_magazine.size(),
				m_bFireSingleShot?"true":"false",
				m_iQueueSize,
				m_iShotNum);
				}
				*/
		if (iAmmoElapsed == 0)
			OnMagazineEmpty();

		StopShooting();
	}
	else
	{
		fShotTimeCounter -= dt;
	}
}

void CWeaponMagazined::state_Misfire(float dt)
{
	OnEmptyClick();
	SwitchState(eIdle);

	bMisfire = true;

	UpdateSounds();
}

void CWeaponMagazined::state_MagEmpty(float dt)
{}

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

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eReload:	ReloadMagazine();	SwitchState(eIdle);	break;	// End of reload animation
	case eHiding:	SwitchState(eHidden);   break;	// End of Hide
	case eShowing:	SwitchState(eIdle);		break;	// End of Show
	case eIdle:		switch2_Idle();			break;  // Keep showing idle
	}
	inherited::OnAnimationEnd(state);
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

void CWeaponMagazined::switch2_Empty()
{
	OnZoomOut();
}

void CWeaponMagazined::PlayReloadSound()
{
	if (m_sounds_enabled)
	{
		if (bMisfire)
		{
			//TODO: make sure correct sound is loaded in CWeaponMagazined::Load(LPCSTR section)
			if (m_sounds.FindSoundItem("sndReloadMisfire", false))
				PlaySound				("sndReloadMisfire", get_LastFP());
			else
				PlaySound				("sndReload", get_LastFP());
		}
		else
		{
			if (!iAmmoElapsed && !(m_pMagazine && !m_pNextMagazine) && m_sounds.FindSoundItem("sndReloadEmpty", false))
				PlaySound				("sndReloadEmpty", get_LastFP());
			else
				PlaySound				("sndReload", get_LastFP());
		}
	}
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();

	PlayReloadSound();
	PlayAnimReload();
	SetPending(TRUE);
}
void CWeaponMagazined::switch2_Hiding()
{
	OnZoomOut();
	CWeapon::FireEnd();

	if (m_sounds_enabled)
		PlaySound("sndHide", get_LastFP());

	PlayAnimHide();
	SetPending(TRUE);
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
	if (m_sounds_enabled)
		PlaySound("sndShow", get_LastFP());

	SetPending(TRUE);
	PlayAnimShow();
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

void CWeaponMagazined::PlayAnimBore()
{
	if (iAmmoElapsed == 0 && HudAnimationExist("anm_bore_empty"))
		PlayHUDMotion("anm_bore_empty", TRUE, this, GetState());
	else
		inherited::PlayAnimBore();
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

void CWeaponMagazined::PlayAnimShow()
{
	VERIFY(GetState() == eShowing);
	PlayHUDMotion("anm_show", FALSE, this, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
	VERIFY(GetState() == eHiding);
	PlayHUDMotion("anm_hide", TRUE, this, GetState());
}

void CWeaponMagazined::PlayAnimReload()
{
	VERIFY								(GetState() == eReload);

	if (bMisfire)
	{
		if (HudAnimationExist("anm_reload_misfire"))
			PlayHUDMotion				("anm_reload_misfire", TRUE, this, GetState());
		else
			PlayHUDMotion				("anm_reload", TRUE, this, GetState());
	}
	else
	{
		bool detach						= m_pMagazine && !m_pNextMagazine;
		if (HudAnimationExist("anm_reload_empty"))
		{
			if (iAmmoElapsed || detach)
				PlayHUDMotion			("anm_reload", TRUE, this, GetState(), m_ReloadHalfPoint, detach);
			else
				PlayHUDMotion			("anm_reload_empty", TRUE, this, GetState(), m_ReloadEmptyHalfPoint, detach);
		}
		else
		{
			if (iAmmoElapsed)
				PlayHUDMotion			("anm_reload", TRUE, this, GetState(), m_ReloadEmptyHalfPoint, detach, m_ReloadPartialPoint);
			else
				PlayHUDMotion			("anm_reload", TRUE, this, GetState(), m_ReloadEmptyHalfPoint, detach);
		}
	}
}

void CWeaponMagazined::PlayAnimAim()
{
	PlayHUDMotion("anm_idle_aim", TRUE, NULL, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
	if (GetState() != eIdle)
		return;
	if (ADS())
		PlayAnimAim();
	else
		inherited::PlayAnimIdle();
}

void CWeaponMagazined::PlayAnimShoot()
{
	VERIFY(GetState() == eFire);

	if (IsZoomed() && HudAnimationExist("anm_shots_aim"))
		PlayHUDMotion("anm_shots_aim", FALSE, this, GetState());
	else
		PlayHUDMotion("anm_shots", FALSE, this, GetState());
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
bool CWeaponMagazined::SwitchMode()
{
	if (eIdle != GetState() || IsPending()) return false;

	if (SingleShotMode())
		m_iQueueSize = WEAPON_ININITE_QUEUE;
	else
		m_iQueueSize = 1;

	PlaySound("sndEmptyClick", get_LastFP());

	return true;
}

void	CWeaponMagazined::OnNextFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnPrevFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnH_A_Chield()
{
	if (m_bHasDifferentFireModes)
	{
		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (!actor) SetQueueSize(-1);
		else SetQueueSize(GetCurrentFireMode());
	};
	inherited::OnH_A_Chield();
};

void	CWeaponMagazined::SetQueueSize(int size)
{
	m_iQueueSize = size;
};

float	CWeaponMagazined::GetWeaponDeterioration()
{
	// modified by Peacemaker [17.10.08]
	//	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
	//		return inherited::GetWeaponDeterioration();
	//	return m_iShotNum*conditionDecreasePerShot;
	return (m_iShotNum == 1) ? conditionDecreasePerShot : conditionDecreasePerQueueShot;
};

void CWeaponMagazined::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(m_iQueueSize, output_packet);
	save_data(m_iShotNum, output_packet);
	save_data(m_iCurFireMode, output_packet);
}

void CWeaponMagazined::load(IReader &input_packet)
{
	inherited::load(input_packet);
	load_data(m_iQueueSize, input_packet); SetQueueSize(m_iQueueSize);
	load_data(m_iShotNum, input_packet);
	load_data(m_iCurFireMode, input_packet);
}

void CWeaponMagazined::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);

	P.w_u8(u8(m_iCurFireMode & 0x00ff));
}

void CWeaponMagazined::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

	m_iCurFireMode = P.r_u8();
	SetQueueSize(GetCurrentFireMode());
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
		if (m_iQueueSize == WEAPON_ININITE_QUEUE)
		{
			info.fire_mode = "A";
		}
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

	info.icon = m_ammoTypes[m_ammoType];
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

	inherited::FireBullet(pos, shot_dir, fire_disp, cartridge, parent_id, weapon_id, send_hit, GetAmmoElapsed());
}

bool CWeaponMagazined::CanTrade() const
{
	if (iAmmoElapsed)
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

void CWeaponMagazined::OnMotionHalf()
{
	if (ParentIsActor() && GetState() == eReload && m_pMagazineSlot)
	{
		if (m_pMagazine)
			m_pMagazine->Transfer		(parent_id());

		if (m_pNextMagazine)
			m_pMagazineSlot->attachLoadingAddon(m_pNextMagazine->cast<CAddon*>());
	}
}

void CWeaponMagazined::OnHiddenItem()
{
	if (m_pMagazineSlot && m_pMagazineSlot->loading_addon)
		m_pMagazineSlot->detachLoadingAddon();
	inherited::OnHiddenItem				();
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
	if (attach)
	{
		m_pMagazine						= mag;
		if (Chamber() < m_magazine.size())
		{ 
			while (Chamber() < m_magazine.size())
				m_magazine.pop_back		();
		}
		else
			iAmmoElapsed				+= m_pMagazine->Amount();

		if (m_magazine.size())
			FindAmmoClass				(*m_magazine.back().m_ammoSect, true);
		else if (m_pMagazine)
			LoadCartridgeFromMagazine	(!Chamber());

		if (m_pMagazineSlot)
			m_pMagazineSlot->detachLoadingAddon();
	}
	else
	{ 
		iAmmoElapsed					-= m_pMagazine->Amount();
		m_pMagazine						= NULL;
	}
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
				if (!m_selected_scopes[i])
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

	if (addon->MotionSuffix().size())
	{
		m_MotionsSuffix			= (attach) ? addon->MotionSuffix() : 0;
		PlayAnimIdle			();
	}

	if (pSettings->line_exist(addon->Section(), "grip"))
		m_grip_accuracy_modifier = readAccuracyModifier((attach) ? *addon->Section() : *m_section_id, "grip");
			
	if (addon->getSlot()->blocking_iron_sights == 2 || (addon->getSlot()->blocking_iron_sights == 1 && !addon->isLowProfile()))
	{
		m_iron_sights_blockers	+= (attach) ? 1 : -1;
		UpdateBonesVisibility	();
		if (m_lower_iron_sights_on_block)
			SetInvIconType		(!!m_iron_sights_blockers);
	}

	if (auto addon_ao = addon->Cast<CAddonOwner*>())
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
			process_addon				((CAddon*)data, !!param);
			break;
		}

		case eWeight:
			return						inherited::Aboba(type, data, param) + GetMagazineWeight(m_magazine);

		case eTransferAddon:
		{
			CMagazine* mag				= Cast<CMagazine*>((CAddon*)data);
			if (mag)
			{
				StartReload				((param) ? mag->cast<CObject*>() : NULL);
				return					1.f;
			}
			break;
		}

		case eUpdateHudBonesVisibility:
			UpdateHudBonesVisibility	();
			break;

		case eUpdateSlotsTransform:
		{
			float res					= inherited::Aboba(type, data, param);
			if (auto scope = getActiveScope())
				if (scope->Type() == eOptics)
					scope->updateCameraLenseOffset();
			return						res;
		}
	}

	return								inherited::Aboba(type, data, param);
}

void CWeaponMagazined::UpdateSndShot()
{
	if (ParentIsActor())
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

Fvector CWeaponMagazined::getFullFireDirection()
{
	auto hi								= HudItemData();
	if (!hi)
		return							get_LastFD();

	float distance						= Zeroing();
	Fvector transference				= m_hud->getMuzzleSightOffset().mad(vForward, distance);
	hi->m_transform.transform_dir		(transference);
	CCartridge cartridge				= m_magazine.back();
	float air_resistance_correction		= Level().BulletManager().CalcZeroingCorrection(cartridge.param_s.fAirResistZeroingCorrection, distance);
	float speed							= m_fStartBulletSpeed * m_silencer_koef.bullet_speed * cartridge.param_s.kBulletSpeed * air_resistance_correction;

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
