#pragma once

#include "WeaponCustomPistol.h"
#include "script_export_space.h"

class CUIFrameWindow;
class CUIStatic;
class CBinocularsVision;

class CWeaponBinoculars: public CWeaponCustomPistol
{
private:
	typedef CWeaponCustomPistol inherited;
protected:
	bool			m_bVision;
public:
					CWeaponBinoculars	(); 
	virtual			~CWeaponBinoculars	();

	void			Load				(LPCSTR section);

	virtual void	net_Destroy			();
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	bool			can_kill			() const;

	virtual bool	Action				(u16 cmd, u32 flags);
	virtual void	UpdateCL			();
	virtual void	render_item_ui		();
	virtual bool	render_item_ui_query();
	virtual bool	use_crosshair		()	const {return false;}
	virtual void	net_Relcase			(CObject *object);

protected:
	CBinocularsVision*					m_binoc_vision;

	DECLARE_SCRIPT_REGISTER_FUNCTION

private:
	void								setADS								O$	(int mode);
};

add_to_type_list(CWeaponBinoculars)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBinoculars)
