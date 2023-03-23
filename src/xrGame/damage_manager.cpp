////////////////////////////////////////////////////////////////////////////
//	Module 		: damage_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Damage manager
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "damage_manager.h"
#include "../xrEngine/xr_object.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrEngine/bone.h"

CDamageManager::CDamageManager			()
{
}

CDamageManager::~CDamageManager			()
{
}

DLL_Pure *CDamageManager::_construct	()
{
	m_object				= smart_cast<CObject*>(this);
	VERIFY					(m_object);
	return					(m_object);
}

void CDamageManager::reload				(LPCSTR section, CInifile const * ini)
{
	m_default_hit_factor	= 1.f;
	m_default_wound_factor	= 1.f;

	bool section_exist		= ini && ini->section_exist(section);
	
	// прочитать дефолтные параметры
	if (section_exist) {
		string32 buffer;
		if (ini->line_exist(section,"default")) {
			LPCSTR value			= ini->r_string(section,"default");
			m_default_hit_factor	= (float)atof(_GetItem(value,0,buffer));
			m_default_wound_factor  = (float)atof(_GetItem(value,2,buffer));
		}
	}

	//инициализировать default параметрами
	init_bones		(section,ini);

	// записать поверху прописанные параметры
	if (section_exist) {
		load_section	(section,ini);
	}
}

void CDamageManager::reload(LPCSTR section, LPCSTR line, CInifile const * ini)
{
	if (ini && ini->section_exist(section) && ini->line_exist(section,line)) 
		reload(ini->r_string(section,line),ini);	
	else 
		reload(section,0);
}

void CDamageManager::init_bones(LPCSTR section, CInifile const * ini)
{
	IKinematics				*kinematics = smart_cast<IKinematics*>(m_object->Visual());
	VERIFY					(kinematics);
	for(u16 i = 0; i<kinematics->LL_BoneCount(); i++)
	{
		CBoneInstance			&bone_instance = kinematics->LL_GetBoneInstance(i);
		bone_instance.set_param	(0,m_default_hit_factor);
		bone_instance.set_param	(1,1.f);
		bone_instance.set_param	(2,m_default_wound_factor);
	}
}
void CDamageManager::load_section(LPCSTR section, CInifile const * ini)
{
	string32						buffer;
	IKinematics*					kinematics = smart_cast<IKinematics*>(m_object->Visual());
	CInifile::Sect&					damages = ini->r_section(section);
	for (CInifile::SectCIt i = damages.Data.begin(), i_e = damages.Data.end(); i != i_e; ++i)
	{
		if (xr_strcmp(*i->first, "default")) // read all except default line
		{
			VERIFY					(m_object);
			int						bone = kinematics->LL_BoneID(i->first);
			R_ASSERT2				(bone != BI_NONE, *i->first);
			CBoneInstance&			bone_instance = kinematics->LL_GetBoneInstance(u16(bone));
			LPCSTR src				= *i->second;
			bone_instance.set_param	(0, (float)atof(_GetItem(src, 0, buffer)));
			bone_instance.set_param	(1, (float)atoi(_GetItem(src, 1, buffer)));
			bone_instance.set_param	(2, (atof(_GetItem(src, 2, buffer)) == 0) ? 0.f : 1.f);
			bone_instance.set_param	(3, (float)atof(_GetItem(src, (_GetItemCount(src) < 4) ? 0 : 3, buffer)));

			if (fis_zero(bone_instance.get_param(0)))
			{
				string256			error_str;
				xr_sprintf			(error_str, "bone_density cannot be zero. see section [%s]", section);
				R_ASSERT2			(0, error_str);
			}
		}
	}
}


void  CDamageManager::HitScale(const u16 element, float& main_damage_scale, float& pierce_damage_scale)
{
	if (element == BI_NONE)
	{
		//считаем что параметры для BI_NONE заданы как 1.f 
		main_damage_scale	= m_default_hit_factor;
		return;
	}

	IKinematics* V			= smart_cast<IKinematics*>(m_object->Visual());
	VERIFY					(V);
	if (pierce_damage_scale == -1.f)
	{
		main_damage_scale	= V->LL_GetBoneInstance(element).get_param(0);
		return;
	}
	float scale				= V->LL_GetBoneInstance(element).get_param(1);
	main_damage_scale		= scale;
	pierce_damage_scale		= pow(scale, 2.f);
}