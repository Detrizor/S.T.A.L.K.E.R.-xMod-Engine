#include "stdafx.h"
#include "WeaponAutomaticShotgun.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"

CWeaponAutomaticShotgun::CWeaponAutomaticShotgun()
{
	m_eSoundClose					= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
	m_eSoundAddCartridge			= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
}

CWeaponAutomaticShotgun::~CWeaponAutomaticShotgun()
{
}

void CWeaponAutomaticShotgun::Load(LPCSTR section)
{
	inherited::Load					(section);

	m_bTriStateReload				= !!READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_reload", false);
	if (m_bTriStateReload)
	{
		m_sounds.LoadSound			(section, "snd_open_weapon", "sndOpen", false, m_eSoundOpen);
		m_sounds.LoadSound			(section, "snd_add_cartridge", "sndAddCartridge", false, m_eSoundAddCartridge);
		m_sounds.LoadSound			(section, "snd_close_weapon", "sndClose", false, m_eSoundClose);
	}
}

bool CWeaponAutomaticShotgun::Action(u16 cmd, u32 flags) 
{
	if (inherited::Action			(cmd, flags))
		return						true;

	if (m_bTriStateReload && GetState() == eReload && cmd == kWPN_FIRE && flags&CMD_START && m_sub_state == eSubstateReloadInProcess) //остановить перезагрузку
	{
		m_sub_state					= eSubstateReloadEnd;
		return						true;
	}
	return							false;
}

void CWeaponAutomaticShotgun::OnAnimationEnd(u32 state) 
{
	if (!m_bTriStateReload || state != eReload)
		return						inherited::OnAnimationEnd(state);

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		m_sub_state					= eSubstateReloadInProcess;
		SwitchState					(eReload);
		break;
	case eSubstateReloadInProcess:
		if (!AddCartridge())
			m_sub_state				= eSubstateReloadEnd;
		SwitchState					(eReload);
		break;
	case eSubstateReloadEnd:
		m_sub_state					= eSubstateReloadBegin;
		SwitchState					(eIdle);
		break;
	}
}

void CWeaponAutomaticShotgun::StartReload()
{
	if (m_bTriStateReload)
	{
		CWeapon::Reload					();
		m_sub_state						= eSubstateReloadBegin;
		SwitchState						(eReload);
	}
	else
		inherited::StartReload			();
}

void CWeaponAutomaticShotgun::OnStateSwitch	(u32 S, u32 oldState)
{
	if (!m_bTriStateReload || S != eReload)
	{
		inherited::OnStateSwitch	(S, oldState);
		return;
	}

	CWeapon::OnStateSwitch			(S, oldState);

	if (GetAmmoElapsed() == GetAmmoMagSize() || !HaveCartridgeInInventory())
	{
		switch2_EndReload			();
		m_sub_state					= eSubstateReloadEnd;
		return;
	}

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		if (HaveCartridgeInInventory())
			switch2_StartReload		();
		break;
	case eSubstateReloadInProcess:
		if (HaveCartridgeInInventory())
			switch2_AddCartgidge	();
		break;
	case eSubstateReloadEnd:
		switch2_EndReload			();
		break;
	}
}

void CWeaponAutomaticShotgun::switch2_StartReload()
{
	PlaySound						("sndOpen", get_LastFP());
	PlayHUDMotion					("anm_open", FALSE, GetState());
	SetPending						(TRUE);
}

void CWeaponAutomaticShotgun::switch2_AddCartgidge()
{
	PlaySound						("sndAddCartridge", get_LastFP());
	PlayHUDMotion					("anm_add_cartridge", FALSE, GetState());
	SetPending						(TRUE);
}

void CWeaponAutomaticShotgun::switch2_EndReload()
{
	PlaySound						("sndClose", get_LastFP());
	PlayHUDMotion					("anm_close", FALSE, GetState());
	SetPending						(FALSE);
}

bool CWeaponAutomaticShotgun::HaveCartridgeInInventory(u8 cnt)
{
	if (unlimited_ammo())
		return						true;
	if (!m_pInventory)
		return						false;

	if (ParentIsActor())
		return						(m_pCartridgeToReload && m_pCartridgeToReload->GetAmmoCount() >= cnt);
	else
	{
		u32 ac						= GetAmmoCount(m_ammoType);
		if (ac < cnt)
		{
			for (u8 i = 0, i_e = u8(m_ammoTypes.size()); i < i_e; ++i)
			{
				if (m_ammoType == i)
					continue;
				ac					+= GetAmmoCount(i);
				if (ac >= cnt)
				{
					m_ammoType		= i;
					break;
				}
			}
		}
		return						(ac >= cnt);
	}
}

bool CWeaponAutomaticShotgun::AddCartridge()
{
	if (!HaveCartridgeInInventory())
		return						false;

	CWeaponAmmo*					cartridges;
	if (ParentIsActor())
	{
		if (!m_pCartridgeToReload)
			return					false;

		cartridges					= m_pCartridgeToReload;
		set_ammo_type				(cartridges->m_section_id);
	}
	else
	{
		if (m_set_next_ammoType_on_reload != undefined_ammo_type)
		{
			m_ammoType				= m_set_next_ammoType_on_reload;
			m_set_next_ammoType_on_reload = undefined_ammo_type;
		}
		cartridges					= smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));
	}

	if (get_ammo_type(m_DefaultCartridge.m_ammoSect) != m_ammoType)
		m_DefaultCartridge.Load		(m_ammoTypes[m_ammoType].c_str());

	CCartridge l_cartridge			= m_DefaultCartridge;
	if (unlimited_ammo() || (cartridges && cartridges->Get(l_cartridge)))
	{
		l_cartridge.m_fCondition	= cartridges->GetCondition();
		if (m_chamber.size() < m_chamber.capacity())
			m_chamber.push_back		(l_cartridge);
		else
			m_magazin.push_back		(l_cartridge);
	}
	else
		return						false;

	return							true;
}

void CWeaponAutomaticShotgun::net_Export(NET_Packet& P)
{
	inherited::net_Export			(P);	
	P.w_u8							(u8(m_magazin.size()));	
	for (u32 i = 0; i < m_magazin.size(); i++)
		P.w_u8						(get_ammo_type(m_magazin[i].m_ammoSect));
}

void CWeaponAutomaticShotgun::net_Import(NET_Packet& P)
{
	inherited::net_Import			(P);
	u8 AmmoCount = P.r_u8			();
	for (u32 i = 0; i < AmmoCount; i++)
	{
		u8 LocalAmmoType = P.r_u8	();
		if (i >= m_magazin.size())
			continue;
		CCartridge& l_cartridge		= m_magazin[i];
		if (m_ammoTypes[LocalAmmoType] == l_cartridge.m_ammoSect)
			continue;
		l_cartridge.Load			(m_ammoTypes[LocalAmmoType].c_str());
	}
}
