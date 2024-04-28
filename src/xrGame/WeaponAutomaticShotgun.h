#pragma once
#include "WeaponMagazined.h"
#include "script_export_space.h"

class CWeaponAutomaticShotgun :	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponAutomaticShotgun			();

	virtual void	Load							(LPCSTR section);
	
	virtual void	net_Export						(NET_Packet& P);
	virtual void	net_Import						(NET_Packet& P);

	virtual bool	Action							(u16 cmd, u32 flags);

protected:
	void	OnAnimationEnd							(u32 state) override;
	void	PlayAnimReload							() override;

	ESoundTypes		m_eSoundOpen;
	ESoundTypes		m_eSoundAddCartridge;
	ESoundTypes		m_eSoundClose;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CWeaponAutomaticShotgun)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAutomaticShotgun)
