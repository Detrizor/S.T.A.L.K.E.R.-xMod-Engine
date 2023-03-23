#include "pch_script.h"

#include "WeaponMagazined.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "scope.h"
#include "silencer.h"
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
#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "HudSound.h"

#include "../build_config_defines.h"

ENGINE_API	bool	g_dedicated_server;

CUIXml*				pWpnScopeXml = NULL;

void createWpnScopeXML()
{
    if (!pWpnScopeXml)
    {
        pWpnScopeXml = xr_new<CUIXml>();
        pWpnScopeXml->Load(CONFIG_PATH, UI_PATH, "scopes.xml");
    }
}

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
	m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

	m_pCurrentAmmo = NULL;

    m_bFireSingleShot = false;
    m_iShotNum = 0;
    m_fOldBulletSpeed = 0.f;
    m_iQueueSize = WEAPON_ININITE_QUEUE;
    m_bLockType = false;
	m_bHasChamber = false;
	m_bHasDifferentFireModes = false;
	m_iCurFireMode = -1;
	m_iPrefferedFireMode = -1;

	m_toReloadID = u16(-1);
}

CWeaponMagazined::~CWeaponMagazined()
{
    // sounds
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

	//Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
	m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
	//if (WeaponSoundExist(section, "snd_shoot_actor"))
	//	m_layered_sounds.LoadSound(section, "snd_shoot_actor", "sndShotActor", false, m_eSoundShot);
#else
	m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
	//if (WeaponSoundExist(section, "snd_shoot_actor"))
	//	m_sounds.LoadSound(section, "snd_shoot_actor", "sndShot", false, m_eSoundShot);
#endif
	//-Alundaio

    m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", true, m_eSoundEmptyClick);
    m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    if (WeaponSoundExist(section, "snd_reload_empty"))
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    if (WeaponSoundExist(section, "snd_reload_misfire"))
        m_sounds.LoadSound(section, "snd_reload_misfire", "sndReloadMisfire", true, m_eSoundReloadMisfire);
#endif //-NEW_SOUNDS

    m_sSndShotCurrent = IsSilencerAttached() ? "sndSilencerShot" : "sndShot";

    //звуки и партиклы глушителя, еслит такой есть
    if (m_eSilencerStatus != ALife::eAddonDisabled)
    {
        if (pSettings->line_exist(section, "silencer_flame_particles"))
            m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
        if (pSettings->line_exist(section, "silencer_smoke_particles"))
            m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");

		//Alundaio: LAYERED_SND_SHOOT Silencer
#ifdef LAYERED_SND_SHOOT
		m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
		if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
			m_layered_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
#else
		m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
		if (WeaponSoundExist(section, "snd_silncer_shot_actor"))
		m_sounds.LoadSound(section, "snd_silncer_shot_actor", "sndSilencerShotActor", false, m_eSoundShot);
#endif
		//-Alundaio

    }

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
    LoadSilencerKoeffs();

	m_bHasChamber = !!READ_IF_EXISTS(pSettings, r_bool, section, "has_chamber", true);
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
			if (m_bHasChamber)
			{
				CInventoryOwner* IO = smart_cast<CInventoryOwner*>(Parent);
				CCartridge& cartridge = m_magazine.back();
				IO->GiveAmmo(*cartridge.m_ammoSect);
				iAmmoElapsed -= 1;
				m_magazine.pop_back();
				m_pInventory->Ruck(this);
				if (IsMisfire())
					bMisfire = false;
			}
			else if (IsMisfire())
			{
				m_pInventory->Ruck(this);
				bMisfire = false;
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

void CWeaponMagazined::StartReload()
{
	CWeapon::Reload();
	SetPending(TRUE);
	SwitchState(eReload);
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

void CWeaponMagazined::Discharge(bool spawn_ammo)
{
	if (ParentIsActor())
	{
#ifdef	EXTENDED_WEAPON_CALLBACKS
		int	AC = GetSuitableAmmoTotal();
		Actor()->callback(GameObject::eOnWeaponMagazineEmpty)(lua_game_object(), AC);
#endif
	}

	xr_map<LPCSTR, u16> l_ammo;

	while (!m_magazine.empty())
	{
		CCartridge &l_cartridge = m_magazine.back();
		xr_map<LPCSTR, u16>::iterator l_it;
		for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
		{
			if (!xr_strcmp(*l_cartridge.m_ammoSect, l_it->first))
			{
				++(l_it->second);
				break;
			}
		}

		if (l_it == l_ammo.end()) l_ammo[*l_cartridge.m_ammoSect] = 1;
		m_magazine.pop_back();
		--iAmmoElapsed;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (!spawn_ammo)
		return;

	xr_map<LPCSTR, u16>::iterator l_it;
	for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
	{
		if (l_it->second && !unlimited_ammo())
			SpawnAmmo(l_it->second, l_it->first);
	}
}

void CWeaponMagazined::UnloadMagazine()
{
	shared_str& mag_section		= m_magazineTypes[iMagazineIndex - 1];
	float cond					= float(MagazineElapsed() - Chamber()) / pSettings->r_float(mag_section, "max_uses");
	LPCSTR custom_data			= GetMagazine(false);

	luabind::functor<u16>				funct;
	ai().script_engine().functor		("xmod_magazine_manager.give_object_custom", funct);
	funct								(*mag_section, cond, custom_data, false, true);

	bool chamber						= !!Chamber();
	CCartridge							l_cartridge;
	xr_vector<CCartridge>& magazine		= Magazine();
	if (chamber)
		l_cartridge						= magazine.back();
	while (magazine.size())
		magazine.pop_back				();
	if (chamber)
		magazine.push_back				(l_cartridge);
	iAmmoElapsed						= m_magazine.size();
	iMagazineIndex						= 0;
}

bool CWeaponMagazined::LoadMagazine(CEatableItem* mag)
{
	if (FindMagazineClass(*mag->m_section_id))
	{
		LPCSTR data			= mag->CustomData().c_str();
		u32 size			= m_ammoTypes.size();
		for (u32 i = 0; data[i]; ++i)
		{
			if (u32(data[i] - '0') > size)
				return		false;
		}
		m_toReloadID		= mag->object_id();
		StartReload			();
		return				true;
	}
	return					false;
}

bool CWeaponMagazined::LoadCartridge(CWeaponAmmo* cartridge)
{
	u8 ammo_type								= FindAmmoClass(*cartridge->m_section_id);
	if (ammo_type)
	{
		if (m_magazineTypes.size())
		{
			if (iAmmoElapsed == 0)
			{
				SetAmmoType						(ammo_type);
				CCartridge						l_cartridge;
				l_cartridge.Load				(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
				m_magazine.push_back			(l_cartridge);
				++iAmmoElapsed;
				--cartridge->m_boxCurr;
				if (!cartridge->m_boxCurr)
					cartridge->SetDropManual	(TRUE);
				m_pInventory->Ruck				(this);
				return							true;
			}
		}
		else if (iAmmoElapsed != iMagazineSize)
		{
			m_toReloadID						= cartridge->object_id();
			StartReload							();
			return								true;
		}
	}
	return										false;
}

void CWeaponMagazined::ReloadMagazine()
{
	if (ParentIsActor())
	{
		PIItem to_reload = GetToReload();
		if (!to_reload)
			return;

		if (to_reload->m_main_class == "magazine")
		{
			u8 mag_idx								= FindMagazineClass(*to_reload->m_section_id);
			LPCSTR custom_data						= *smart_cast<CEatableItem*>(to_reload)->CustomData();
			to_reload->object().DestroyObject		();
			if (iMagazineIndex)
				UnloadMagazine						();
			iMagazineIndex							= mag_idx;
			SetMagazine								(custom_data, false);
		}
		else
		{
			FindAmmoClass(*to_reload->m_section_id, true);
			CWeaponAmmo* cartridges = smart_cast<CWeaponAmmo*>(to_reload);
			CCartridge l_cartridge;
			while (iAmmoElapsed < iMagazineSize)
			{
				if (!unlimited_ammo() && !cartridges->Get(l_cartridge))
					break;
				l_cartridge.m_LocalAmmoType = m_ammoType;
				m_magazine.push_back(l_cartridge);
				++iAmmoElapsed;
			}
			if (cartridges && !cartridges->m_boxCurr && OnServer())
				to_reload->object().DestroyObject();
		}
		m_toReloadID = u16(-1);
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
		if (!m_pCurrentAmmo && !unlimited_ammo()) return;

		//разрядить магазин, если загружаем патронами другого типа
		if (!m_bLockType && !m_magazine.empty() &&
			(!m_pCurrentAmmo || xr_strcmp(m_pCurrentAmmo->cNameSect(),
			*m_magazine.back().m_ammoSect)))
			Discharge();

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
		if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
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

u8 CWeaponMagazined::FindAmmoClass(LPCSTR section, bool set)
{
	for (u8 i = 0, i_e = u8(m_ammoTypes.size()); i < i_e; ++i)
	{
		if (m_ammoTypes[i] == section)
		{
			if (set)
				m_ammoType = i;
			return i + 1;
		}
	}
	return 0;
}

u8 CWeaponMagazined::FindMagazineClass(LPCSTR section, bool set)
{
	for (u8 i = 0, i_e = u8(m_magazineTypes.size()); i < i_e; ++i)
	{
		if (m_magazineTypes[i] == section)
		{
			if (set)
				iMagazineIndex = i + 1;
			return i + 1;
		}
	}
	return 0;
}

LPCSTR CWeaponMagazined::GetMagazine(bool with_chamber)
{
	shared_str							data;
	data._set							("");
	xr_vector<CCartridge>& magazine		= Magazine();
	for (u32 i = 0, i_e = u32((with_chamber) ? MagazineElapsed() : MagazineElapsed() - Chamber()); i != i_e; i++)
		data.printf						("%s%c", *data, '0' + magazine[i].m_LocalAmmoType + 1);

	return *data;
}

xr_vector<CCartridge>& CWeaponMagazined::Magazine()
{
	return m_magazine;
}

u32 CWeaponMagazined::MagazineElapsed()
{
	return Magazine().size();
}

void CWeaponMagazined::SetMagazine(LPCSTR data, bool with_chamber)
{
	bool chamber				= !!Chamber();
	CCartridge					chamber_cartridge;
	if (!with_chamber && chamber)
	{
		chamber_cartridge		= m_magazine.back();
		m_magazine.pop_back		();
	}

	while (m_magazine.size())
		m_magazine.pop_back		();
	iAmmoElapsed				= 0;
	CCartridge					l_cartridge;
	for (u32 i = 0; data[i]; ++i)
	{
		SetAmmoType				(data[i] - '0');
		l_cartridge.Load		(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
		m_magazine.push_back	(l_cartridge);
		++iAmmoElapsed;
	}

	if (!with_chamber && chamber)
	{
		m_ammoType				= chamber_cartridge.m_LocalAmmoType;
		m_magazine.push_back	(chamber_cartridge);
		++iAmmoElapsed;
	}
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

		Fvector p1, d;
        p1.set(get_LastFP());
        d.set(get_LastFD());
		
        
        E->g_fireParams(this, p1, d);
		
        if (m_iShotNum == 0)
        {
            m_vStartPos = p1;
            m_vStartDir = d;
        }

        VERIFY(!m_magazine.empty());

        while (!m_magazine.empty() && fShotTimeCounter < 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
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

            OnShot();

            if (m_iShotNum > m_iBaseDispersionedBulletsCount)
                FireTrace(p1, d);
            else
                FireTrace(m_vStartPos, m_vStartDir);
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
    // SoundWeaponMagazined.cpp


	
//Alundaio: LAYERED_SND_SHOOT
#ifdef LAYERED_SND_SHOOT
	//Alundaio: Actor sounds
	if (ParentIsActor())
	{
		/*
		if (strcmp(m_sSndShotCurrent.c_str(), "sndShot") == 0 && pSettings->line_exist(m_section_id,"snd_shoot_actor") && m_layered_sounds.FindSoundItem("sndShotActor", false))
			m_layered_sounds.PlaySound("sndShotActor", get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
		else if (strcmp(m_sSndShotCurrent.c_str(), "sndSilencerShot") == 0 && pSettings->line_exist(m_section_id,"snd_silncer_shot_actor") && m_layered_sounds.FindSoundItem("sndSilencerShotActor", false))
			m_layered_sounds.PlaySound("sndSilencerShotActor", get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
		else 
		*/
			m_layered_sounds.PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
	} else
		m_layered_sounds.PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), H_Root(), !!GetHUDmode(), false, (u8)-1);
#else
	//Alundaio: Actor sounds
	if (ParentIsActor())
	{
		/*
		if (strcmp(m_sSndShotCurrent.c_str(), "sndShot") == 0 && pSettings->line_exist(m_section_id, "snd_shoot_actor")&& snd_silncer_shot m_sounds.FindSoundItem("sndShotActor", false))
			PlaySound("sndShotActor", get_LastFP(), (u8)(m_iShotNum - 1));
		else if (strcmp(m_sSndShotCurrent.c_str(), "sndSilencerShot") == 0 && pSettings->line_exist(m_section_id, "snd_silncer_shot_actor") && m_sounds.FindSoundItem("sndSilencerShotActor", false))
			PlaySound("sndSilencerShotActor", get_LastFP(), (u8)(m_iShotNum - 1));
		else
		*/
			PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), (u8)-1);
	}
	else
		PlaySound(m_sSndShotCurrent.c_str(), get_LastFP(), (u8)-1); //Alundaio: Play sound at index (ie. snd_shoot, snd_shoot1, snd_shoot2, snd_shoot3)
#endif
//-Alundaio

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
#ifdef NEW_SOUNDS //AVO: use custom sounds
        if (bMisfire)
        {
            //TODO: make sure correct sound is loaded in CWeaponMagazined::Load(LPCSTR section)
            if (m_sounds.FindSoundItem("sndReloadMisfire", false))
                PlaySound("sndReloadMisfire", get_LastFP());
            else
                PlaySound("sndReload", get_LastFP());
        }
        else
        {
            if (iAmmoElapsed == 0)
            {
                if (m_sounds.FindSoundItem("sndReloadEmpty", false))
                    PlaySound("sndReloadEmpty", get_LastFP());
                else
                    PlaySound("sndReload", get_LastFP());
            }
            else
                PlaySound("sndReload", get_LastFP());
        }
#else
        PlaySound("sndReload", get_LastFP());
#endif //-AVO
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
    if (inherited::Action(cmd, flags)) return true;

    //если оружие чем-то занято, то ничего не делать
    if (IsPending()) return false;

    switch (cmd)
    {
    case kWPN_RELOAD:
    {
        if (flags&CMD_START)
			Reload();
    }
    return true;
    case kWPN_FIREMODE_PREV:
    {
        if (flags&CMD_START)
        {
            OnPrevFireMode();
            return true;
        };
    }break;
    case kWPN_FIREMODE_NEXT:
    {
        if (flags&CMD_START)
        {
            OnNextFireMode();
            return true;
        };
    }break;
    }
    return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
    CScope*				pScope = smart_cast<CScope*>(pIItem);
    CSilencer*			pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher*	pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
		shared_str& sect		= pIItem->m_section_id;
		for (SCOPES_VECTOR_IT it = m_scopes.begin(), it_e = m_scopes.end(); it != it_e; it++)
        {
			if (sect == *it)
                return true;
        }
        return false;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 && (m_sSilencerName == pIItem->object().cNameSect()))
        return true;
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 && (m_sGrenadeLauncherName == pIItem->object().cNameSect()))
        return true;
    else
        return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
    if (m_eScopeStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope))
    {
		for (SCOPES_VECTOR_IT it = m_scopes.begin(), it_e = m_scopes.end(); it != it_e; it++)
        {
            if (*it == item_section_name)
                return true;
        }
        return false;
    }

    else if (m_eSilencerStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) && (m_sSilencerName == item_section_name))
        return true;
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && 0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) && (m_sGrenadeLauncherName == item_section_name))
        return true;
    else
        return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
    bool result = false;

    CScope*				pScope = smart_cast<CScope*>(pIItem);
    CSilencer*			pSilencer = smart_cast<CSilencer*>(pIItem);
    CGrenadeLauncher*	pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

    if (pScope && m_eScopeStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
    {
		for (SCOPES_VECTOR_IT it = m_scopes.begin(), it_e = m_scopes.end(); it != it_e; it++)
        {
            if (*it == pIItem->object().cNameSect())
                m_cur_scope = u8(it - m_scopes.begin());
        }
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
        result = true;
    }
    else if (pSilencer && m_eSilencerStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 && (m_sSilencerName == pIItem->object().cNameSect()))
    {
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
        result = true;
    }
    else if (pGrenadeLauncher && m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 && (m_sGrenadeLauncherName == pIItem->object().cNameSect()))
    {
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
        result = true;
    }

    if (result)
    {
		if (b_send_event && OnServer() && !pScope)
            pIItem->object().DestroyObject();
		else
		{
			luabind::functor<void>funct;
			ai().script_engine().functor("xmod_scopes_manager.hide_scope", funct);
			funct(object_id(), pIItem->object_id());
		}

        UpdateAddonsVisibility();
        InitAddons();

        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::DetachScope(const char* item_section_name, bool b_spawn_item)
{
    bool detached = false;
	for (SCOPES_VECTOR_IT it = m_scopes.begin(); it != m_scopes.end(); it++)
    {
		if (*it == item_section_name)
        {
            m_cur_scope = NULL;
            detached = true;
        }
    }
    return detached;
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item)
{
    if (m_eScopeStatus == ALife::eAddonAttachable && DetachScope(item_section_name, b_spawn_item))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope) == 0)
        {
            Msg("ERROR: scope addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;

		luabind::functor<void>funct;
		ai().script_engine().functor("xmod_scopes_manager.unhide_scope", funct);
		funct(object_id());

        UpdateAddonsVisibility();
        InitAddons();

        return true;
    }
    else if (m_eSilencerStatus == ALife::eAddonAttachable && (m_sSilencerName == item_section_name))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0)
        {
            Msg("ERROR: silencer addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

        UpdateAddonsVisibility();
        InitAddons();
		return CInventoryItemObjectOld::Detach(item_section_name, b_spawn_item);
    }
    else if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable && (m_sGrenadeLauncherName == item_section_name))
    {
        if ((m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0)
        {
            Msg("ERROR: grenade launcher addon already detached.");
            return true;
        }
        m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

        UpdateAddonsVisibility();
        InitAddons();
		return CInventoryItemObjectOld::Detach(item_section_name, b_spawn_item);
    }
    else
        return inherited::Detach(item_section_name, b_spawn_item);;
}

void CWeaponMagazined::InitAddons()
{
    if (IsScopeAttached())
    {
		if (m_eScopeStatus == ALife::eAddonAttachable || m_eScopeStatus == ALife::eAddonPermanent)
        {
			CScope* pScope						= NULL;
			if (m_eScopeStatus == ALife::eAddonAttachable)
			{
				luabind::functor<u16>			funct;
				ai().script_engine().functor	("xmod_scopes_manager.get_scope", funct);
				u16 scope_id					= funct(object_id());
				pScope							= smart_cast<CScope*>(Level().Objects.net_Find(scope_id));
			}
			const shared_str& scope_name		= GetScopeName();

            LPCSTR scope_tex_name						= READ_IF_EXISTS(pSettings, r_string, scope_name, "scope_texture", 0);
            m_zoom_params.m_sUseZoomPostprocess			= READ_IF_EXISTS(pSettings, r_string, scope_name, "scope_nightvision", 0);
			if (pScope)
			{
				m_zoom_params.m_fScopeZoomFactor		= pScope->m_fScopeZoomFactor;
				m_zoom_params.m_fScopeMinZoomFactor		= pScope->m_fScopeMinZoomFactor;
				m_zoom_params.m_sUseBinocularVision		= pScope->m_bScopeAliveDetector;
			}
			else
			{
				if (m_zoom_params.m_fScopeZoomFactor == 1.f)
					m_zoom_params.m_fScopeZoomFactor		= READ_IF_EXISTS(pSettings, r_float, scope_name, "scope_zoom_factor", 1.f);
				if (m_zoom_params.m_fScopeMinZoomFactor == 1.f)
					m_zoom_params.m_fScopeMinZoomFactor		= READ_IF_EXISTS(pSettings, r_float, scope_name, "scope_min_zoom_factor", m_zoom_params.m_fScopeZoomFactor);
				if (!m_zoom_params.m_sUseBinocularVision.size())
					m_zoom_params.m_sUseBinocularVision		= READ_IF_EXISTS(pSettings, r_string, scope_name, "scope_alive_detector", 0);
			}
            m_fRTZoomFactor								= m_zoom_params.m_fScopeMinZoomFactor;
			InitRotateTime								();
            if (m_UIScope)
                xr_delete								(m_UIScope);

			if (!g_dedicated_server && scope_tex_name)
            {
                m_UIScope = xr_new<CUIWindow>();
                createWpnScopeXML();
                CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name, 0, m_UIScope);
            }
			
			shared_str& scope_sect		= m_scopes_sect[m_cur_scope];
			LPCSTR tmp					= READ_IF_EXISTS(pSettings, r_string, scope_sect, "visual", 0);
			if (tmp && cNameVisual() != tmp)
				cNameVisual_set			(tmp);
			tmp							= READ_IF_EXISTS(pSettings, r_string, scope_sect, "hud", 0);
			if (tmp && hud_sect != tmp)
				hud_sect._set			(tmp);
        }
    }
	else
	{
		if (m_UIScope)
			xr_delete(m_UIScope);

		if (IsZoomEnabled())
		{
			m_zoom_params.m_fScopeZoomFactor		= READ_IF_EXISTS(pSettings, r_float, m_section_id, "scope_zoom_factor", 1.f);
			m_zoom_params.m_fScopeMinZoomFactor		= READ_IF_EXISTS(pSettings, r_float, m_section_id, "scope_min_zoom_factor", m_zoom_params.m_fScopeZoomFactor);
			InitRotateTime							();
		}

		LPCSTR tmp				= pSettings->r_string(m_section_id, "visual");
		if (cNameVisual() != tmp)
			cNameVisual_set		(tmp);
		tmp						= pSettings->r_string(m_section_id, "hud");
		if (hud_sect != tmp)
			hud_sect._set		(tmp);
	}

    if (IsSilencerAttached()/* && SilencerAttachable() */)
    {
        m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
        m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
        m_sSndShotCurrent = "sndSilencerShot";

        //подсветка от выстрела
        LoadLights(*cNameSect(), "silencer_");
        ApplySilencerKoeffs();
    }
    else
    {
        m_sFlameParticlesCurrent = m_sFlameParticles;
        m_sSmokeParticlesCurrent = m_sSmokeParticles;
        m_sSndShotCurrent = "sndShot";

        //подсветка от выстрела
        LoadLights(*cNameSect(), "");
        ResetSilencerKoeffs();
    }

    inherited::InitAddons();
}

void CWeaponMagazined::LoadSilencerKoeffs()
{
    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        LPCSTR sect = m_sSilencerName.c_str();
        m_silencer_koef.bullet_speed = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_speed_k", 1.0f);
        m_silencer_koef.fire_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "fire_dispersion_base_k", 1.0f);
        m_silencer_koef.cam_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
        m_silencer_koef.cam_disper_inc = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
    }

    clamp(m_silencer_koef.bullet_speed, 0.0f, 1.0f);
    clamp(m_silencer_koef.fire_dispersion, 0.0f, 3.0f);
    clamp(m_silencer_koef.cam_dispersion, 0.0f, 1.0f);
    clamp(m_silencer_koef.cam_disper_inc, 0.0f, 1.0f);
}

void CWeaponMagazined::ApplySilencerKoeffs()
{
    cur_silencer_koef = m_silencer_koef;
}

void CWeaponMagazined::ResetSilencerKoeffs()
{
    cur_silencer_koef.Reset();
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
    VERIFY(GetState() == eReload);
#ifdef NEW_ANIMS //AVO: use new animations
    if (bMisfire)
    {
        //Msg("AVO: ------ MISFIRE");
        if (HudAnimationExist("anm_reload_misfire"))
            PlayHUDMotion("anm_reload_misfire", TRUE, this, GetState());
        else
            PlayHUDMotion("anm_reload", TRUE, this, GetState());
    }
    else
    {
        if (iAmmoElapsed == 0)
        {
            if (HudAnimationExist("anm_reload_empty"))
                PlayHUDMotion("anm_reload_empty", TRUE, this, GetState());
            else
                PlayHUDMotion("anm_reload", TRUE, this, GetState());
        }
        else
        {
            PlayHUDMotion("anm_reload", TRUE, this, GetState());
        }
    }
#else
    PlayHUDMotion("anm_reload", TRUE, this, GetState());
#endif //-NEW_ANIM
}

void CWeaponMagazined::PlayAnimAim()
{
    PlayHUDMotion("anm_idle_aim", TRUE, NULL, GetState());
}

void CWeaponMagazined::PlayAnimIdle()
{
    if (GetState() != eIdle)	return;
    if (IsZoomed())
    {
        PlayAnimAim();
    }
    else
        inherited::PlayAnimIdle();
}

void CWeaponMagazined::PlayAnimShoot()
{
    VERIFY(GetState() == eFire);
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

    int	ae = GetAmmoElapsed();
    xr_sprintf(int_str, "%d", ae);
    info.cur_ammo = int_str;

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
        info.fmj_ammo._set("--");
        info.ap_ammo._set("--");
		info.third_ammo._set("--"); //Alundaio
    }
    else
    {
		//Alundaio: Added third ammo type and cleanup
		info.fmj_ammo._set("");
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

	info.icon = (ae != 0 && m_magazine.size() != 0) ? m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str() : m_ammoTypes[m_ammoType].c_str();
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

    result2 = process_if_exists(section, "snd_draw", str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_draw", "sndShow", false, m_eSoundShow);
    }
    result |= result2;

    result2 = process_if_exists(section, "snd_holster", str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_holster", "sndHide", false, m_eSoundHide);
    }
    result |= result2;

    result2 = process_if_exists(section, "snd_shoot", str, test);
    if (result2 && !test)
    {
#ifdef LAYERED_SND_SHOOT
		m_layered_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#else
        m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, m_eSoundShot);
#endif
    }
    result |= result2;

    result2 = process_if_exists(section, "snd_empty", str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_empty", "sndEmptyClick", false, m_eSoundEmptyClick);
    }
    result |= result2;

    result2 = process_if_exists(section, "snd_reload", str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload", "sndReload", true, m_eSoundReload);
    }
    result |= result2;

#ifdef NEW_SOUNDS //AVO: custom sounds go here
    result2 = process_if_exists(section, "snd_reload_empty", str, test);
    if (result2 && !test)
    {
        m_sounds.LoadSound(section, "snd_reload_empty", "sndReloadEmpty", true, m_eSoundReloadEmpty);
    }
    result |= result2;
#endif //-NEW_SOUNDS

    if (m_eSilencerStatus != ALife::eAddonDisabled)
    {
        result |= process_if_exists(section, "silencer_flame_particles", m_sSilencerFlameParticles, test);
        result |= process_if_exists(section, "silencer_smoke_particles", m_sSilencerSmokeParticles, test);

        result2 = process_if_exists(section, "snd_silncer_shot", str, test);
        if (result2 && !test)
        {
#ifdef LAYERED_SND_SHOOT
			m_layered_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#else
			m_sounds.LoadSound(section, "snd_silncer_shot", "sndSilencerShot", false, m_eSoundShot);
#endif
        }
        result |= result2;
    }

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