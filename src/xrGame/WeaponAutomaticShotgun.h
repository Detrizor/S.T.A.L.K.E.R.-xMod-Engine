#pragma once
#include "WeaponMagazined.h"
#include "script_export_space.h"

class CWeaponAutomaticShotgun :	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
					CWeaponAutomaticShotgun			();
	virtual			~CWeaponAutomaticShotgun		();

	virtual void	Load							(LPCSTR section);
	
	virtual void	net_Export						(NET_Packet& P);
	virtual void	net_Import						(NET_Packet& P);

	virtual void	Reload							();
	virtual void	StartReload						();
	void			switch2_StartReload				();
	void			switch2_AddCartgidge			();
	void			switch2_EndReload				();

	virtual void	PlayAnimOpenWeapon				();
	virtual void	PlayAnimAddOneCartridgeWeapon	();
	void			PlayAnimCloseWeapon				();

	virtual bool	Action							(u16 cmd, u32 flags);

protected:
	virtual void	OnAnimationEnd					(u32 state);
	void			TriStateReload					();
	virtual void	OnStateSwitch					(u32 S, u32 oldState);

	bool			HaveCartridgeInInventory		(u8 cnt = 1);
	bool			AddCartridge					();

	ESoundTypes		m_eSoundOpen;
	ESoundTypes		m_eSoundAddCartridge;
	ESoundTypes		m_eSoundClose;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CWeaponAutomaticShotgun)
#undef script_type_list
#define script_type_list save_type_list(CWeaponAutomaticShotgun)
