#include "stdafx.h"
#include "BoneProtections.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrEngine/bone.h"
#include "Level.h"

static LPCSTR ARMOR_LEVELS = pSettings->r_string("damage_manager", "armor_levels");

float SBoneProtections::getBoneArmor(s16 bone_id)
{
	storage_it it = m_bones_koeff.find(bone_id);
	if( it != m_bones_koeff.end() )
		return it->second.armor;
	else
		return m_default.armor;
}

float SBoneProtections::getBoneArmorLevel(s16 bone_id)
{
	storage_it it = m_bones_koeff.find(bone_id);
	if (it != m_bones_koeff.end())
		return it->second.level;
	else
		return m_default.level;
}

void SBoneProtections::reload(const shared_str& bone_sect, IKinematics* kinematics)
{
	VERIFY						(kinematics);
	m_bones_koeff.clear			();

	m_default.level				= -1.f;
	m_default.armor				= -1.f;

	CInifile::Sect	&protections = pSettings->r_section(bone_sect);
	for (CInifile::SectCIt i = protections.Data.begin(); protections.Data.end() != i; i++)
	{
		string256				buffer;

		BoneProtection			BP;

		BP.level				= (float)atof(_GetItem(i->second.c_str(), 0, buffer));
		BP.armor				= ComputeArmor(BP.level);
		
		s16	bone_id				= kinematics->LL_BoneID(i->first);
		R_ASSERT2				(BI_NONE != bone_id, i->first.c_str());			
		m_bones_koeff.insert	(mk_pair(bone_id,BP));
	}
}

void SBoneProtections::add(const shared_str& bone_sect, IKinematics* kinematics, float value)
{
	VERIFY(kinematics);

	CInifile::Sect&				protections = pSettings->r_section(bone_sect);
	for(CInifile::SectCIt i=protections.Data.begin(); protections.Data.end()!=i; ++i) 
	{
		string256				buffer;
		s16	bone_id				= kinematics->LL_BoneID(i->first);
		R_ASSERT2				(BI_NONE != bone_id, i->first.c_str());			
		BoneProtection&	BP		= m_bones_koeff[bone_id];
		BP.level				+= (value > 0.f) ? value : (float)atof(_GetItem(*i->second, 0, buffer));
		BP.armor				= ComputeArmor(BP.level);
	}
}

float SBoneProtections::ComputeArmor(float level)
{
	if (level < 0.f)
		return 0.f;

	string256				buffer;
	int level_low			= iFloor(level);
	int level_high			= iCeil(level);
	float armor_low			= (float)atof(_GetItem(ARMOR_LEVELS, level_low, buffer));
	float armor_high		= (float)atof(_GetItem(ARMOR_LEVELS, level_high, buffer));
	float k_level			= level - (float)level_low;

	return (armor_low + (armor_high - armor_low) * k_level);
}