#pragma once

class IKinematics;

struct SBoneProtections{
	struct BoneProtection {
		float		level;
		float		armor;
	};
	typedef xr_map<s16,BoneProtection>		storage_type;
	typedef storage_type::iterator	storage_it;
						SBoneProtections	()								{m_default.level = -1.f; m_default.armor = -1.f;}
	BoneProtection		m_default;
	storage_type		m_bones_koeff;
	void				reload				(const shared_str& outfit_section, IKinematics* kinematics);
	void				add					(const shared_str& bone_sect, IKinematics* kinematics, float value = 0.f);
	float				getBoneArmor		(s16 bone_id);
	float				getBoneArmorLevel	(s16 bone_id);

	float				ComputeArmor		(float level);
	
	xr_vector<float>				S$	s_armor_levels;
	void							S$	loadStaticVariables						();
};
