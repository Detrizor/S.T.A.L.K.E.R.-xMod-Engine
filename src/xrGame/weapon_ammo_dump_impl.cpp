#include "stdafx.h"
#include "WeaponAmmo.h"

void CCartridge::DumpActiveParams(shared_str const & section_name,
								   CInifile & dst_ini) const
{
	dst_ini.w_float	(section_name.c_str(), "k_disp",			param_s.kDisp);
	dst_ini.w_float (section_name.c_str(), "bullet_mass",		param_s.fBulletMass);
	dst_ini.w_bool	(section_name.c_str(), "hollow_point",		param_s.mHollowPoint);
	dst_ini.w_float	(section_name.c_str(), "bullet_resist",		param_s.fBulletResist);
	dst_ini.w_s32	(section_name.c_str(), "k_buckshot",		param_s.buckShot);
}