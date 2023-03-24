#pragma once

enum EHandsOffsets
{
	eRelaxed,
	eArmed,
	eAim,
	eADS,
	eAlt,
	eGL,
	eTotal
};

class CWeaponHud
{
public:
	CWeaponHud() {}
	~CWeaponHud() {}

	void Load(LPCSTR hud_section);

private:
	Fvector							m_hands_offset[2][eTotal];

	struct shooting_params
	{
		bool bShootShake;
		Fvector4 m_shot_max_offset_LRUD;
		Fvector4 m_shot_max_offset_LRUD_aim;
		Fvector2 m_shot_offset_BACKW;
		float m_ret_speed;
		float m_ret_speed_aim;
		float m_min_LRUD_power;
	} m_shooting_params; //--#SM+#--
	
	float							m_lense_offset;

//	V_ type_name_very_long_____________________ method_name_also_very_long______________ (params_various_count____________________) C_ {}
};
