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

	m_bHasChamber					= !!READ_IF_EXISTS(pSettings, r_bool, section, "has_chamber", false);
}

bool CWeaponAutomaticShotgun::Action(u16 cmd, u32 flags) 
{
	if (inherited::Action			(cmd, flags))
		return						true;

	if (m_bTriStateReload && GetState() == eReload && cmd == kWPN_FIRE && flags&CMD_START && m_sub_state == eSubstateReloadInProcess) //остановить перезагрузку
	{
		AddCartridge				();
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

void CWeaponAutomaticShotgun::Reload() 
{
	if (ParentIsActor() || !m_bTriStateReload)
		inherited::Reload			();
	else
		TriStateReload				();
}

void CWeaponAutomaticShotgun::StartReload()
{
	if (m_bTriStateReload)
		TriStateReload				();
	else
		inherited::StartReload();
}

void CWeaponAutomaticShotgun::TriStateReload()
{
	if (iAmmoElapsed < iMagazineSize && HaveCartridgeInInventory())
	{
		CWeapon::Reload				();
		m_sub_state					= eSubstateReloadBegin;
		SwitchState					(eReload);
	}
}

void CWeaponAutomaticShotgun::OnStateSwitch	(u32 S, u32 oldState)
{
	if (!m_bTriStateReload || S != eReload)
	{
		inherited::OnStateSwitch	(S, oldState);
		return;
	}

	CWeapon::OnStateSwitch			(S, oldState);

	if (m_magazine.size() == (u32)iMagazineSize || !HaveCartridgeInInventory())
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
	PlayAnimOpenWeapon				();
	SetPending						(TRUE);
}

void CWeaponAutomaticShotgun::switch2_AddCartgidge()
{
	PlaySound						("sndAddCartridge", get_LastFP());
	PlayAnimAddOneCartridgeWeapon	();
	SetPending						(TRUE);
}

void CWeaponAutomaticShotgun::switch2_EndReload()
{
	SetPending						(FALSE);
	PlaySound						("sndClose", get_LastFP());
	PlayAnimCloseWeapon				();
}

void CWeaponAutomaticShotgun::PlayAnimOpenWeapon()
{
	VERIFY							(GetState() == eReload);
	PlayHUDMotion					("anm_open", FALSE, this, GetState());
}
void CWeaponAutomaticShotgun::PlayAnimAddOneCartridgeWeapon()
{
	VERIFY							(GetState() == eReload);
	PlayHUDMotion					("anm_add_cartridge", FALSE, this, GetState());
}
void CWeaponAutomaticShotgun::PlayAnimCloseWeapon()
{
	VERIFY							(GetState() == eReload);
	PlayHUDMotion					("anm_close", FALSE, this, GetState());
}

bool CWeaponAutomaticShotgun::HaveCartridgeInInventory(u8 cnt)
{
	if (unlimited_ammo())
		return						true;
	if (!m_pInventory)
		return						false;

	if (ParentIsActor())
	{
		PIItem to_reload			= GetToReload();
		CWeaponAmmo* cartridges		= smart_cast<CWeaponAmmo*>(to_reload);
		return						(cartridges && cartridges->m_boxCurr >= cnt);
	}
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
		PIItem to_reload			= GetToReload();
		if (!to_reload)
			return					false;

		FindAmmoClass				(*to_reload->m_section_id, true);
		cartridges					= smart_cast<CWeaponAmmo*>(to_reload);
	}
	else
	{
		if (IsMisfire())
			bMisfire				= false;

		if (m_set_next_ammoType_on_reload != undefined_ammo_type)
		{
			m_ammoType				= m_set_next_ammoType_on_reload;
			m_set_next_ammoType_on_reload = undefined_ammo_type;
		}
		cartridges					= smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));
	}

	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load		(m_ammoTypes[m_ammoType].c_str(), m_ammoType);

	CCartridge l_cartridge			= m_DefaultCartridge;
	if (unlimited_ammo() || (cartridges && cartridges->Get(l_cartridge)))
	{
		++iAmmoElapsed;
		l_cartridge.m_LocalAmmoType	= m_ammoType;
		m_magazine.push_back		(l_cartridge);
	}
	else
		return						false;

	VERIFY							((u32)iAmmoElapsed == m_magazine.size());

	if (cartridges && !cartridges->m_boxCurr && OnServer())
		cartridges->SetDropManual	(TRUE);

	return							true;
}

void CWeaponAutomaticShotgun::net_Export(NET_Packet& P)
{
	inherited::net_Export			(P);	
	P.w_u8							(u8(m_magazine.size()));	
	for (u32 i = 0; i < m_magazine.size(); i++)
	{
		CCartridge& l_cartridge		= *(m_magazine.begin() + i);
		P.w_u8						(l_cartridge.m_LocalAmmoType);
	}
}

void CWeaponAutomaticShotgun::net_Import(NET_Packet& P)
{
	inherited::net_Import			(P);
	u8 AmmoCount = P.r_u8			();
	for (u32 i = 0; i < AmmoCount; i++)
	{
		u8 LocalAmmoType = P.r_u8	();
		if (i >= m_magazine.size())
			continue;
		CCartridge& l_cartridge		= *(m_magazine.begin() + i);
		if (LocalAmmoType == l_cartridge.m_LocalAmmoType)
			continue;
		l_cartridge.Load			(m_ammoTypes[LocalAmmoType].c_str(), LocalAmmoType);
	}
}