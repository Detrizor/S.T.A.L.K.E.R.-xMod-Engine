#include "stdafx.h"
#include "weapon_hud.h"
#include "WeaponMagazined.h"
#include "HUDManager.h"
#include "xr_level_controller.h"
#include "../../xrEngine/xr_input.h"
#include <fstream>
#include "player_hud.h"
#include "Level_Bullet_Manager.h"

const float aim_factor = pSettings->r_float("weapon_manager", "aim_factor");

Fvector2 rotate_vector2(const float& x, const float& y, const float& angle)
{
	Fvector2	res;
	res.x		= x * cos(angle) + y * sin(angle);
	res.y		= y * cos(angle) - x * sin(angle);
	return		res;
}

Fvector2 rotate_vector2(const Fvector2& source, const float& angle)
{
	return		rotate_vector2(source.x, source.y, angle);
}

Fvector rotate_vector3(const Fvector& source, const Fvector& angle)
{
	Fvector res		= source;
	Fvector2 tmp	= rotate_vector2(res.z, res.y, angle.x);
	res.z			= tmp.x;
	res.y			= tmp.y;
	tmp				= rotate_vector2(res.x, res.z, angle.y);
	res.x			= tmp.x;
	res.z			= tmp.y;
	tmp				= rotate_vector2(res.y, res.x, angle.z);
	res.y			= tmp.x;
	res.x			= tmp.y;
	return			res;
}

CWeaponHud::CWeaponHud(CWeaponMagazined* obj) : m_object(obj)
{
	LPCSTR scope = 0;   //--xd temp
	//--xd may have to use eventually m_hands_offset[0][eAlt].add		(rotate_vector3(m_hands_attach[0], m_hands_offset[1][eAlt]));

	shared_str CR$ section = o().HudSection();

	m_hands_offset[0][eArmed]			= pSettings->r_fvector3(section, "armed_pos");
	m_hands_offset[1][eArmed]			= pSettings->r_fvector3(section, "armed_rot");

	if (scope)
	{
		m_hands_offset[0][eADS]			= pSettings->r_fvector3(section, "scope_pos");
		m_hands_offset[1][eADS]			= pSettings->r_fvector3(section, "scope_rot");
		m_hands_offset[0][eADS].add		(pSettings->r_fvector3(scope, "hud_offset_pos"));
		m_hands_offset[1][eADS].add		(pSettings->r_fvector3(scope, "hud_offset_rot"));
	}
	else
	{
		m_hands_offset[0][eADS]			= pSettings->r_fvector3(section, "iron_sights_pos");
		m_hands_offset[1][eADS]			= pSettings->r_fvector3(section, "iron_sights_rot");
	}

	m_lense_offset						= m_hands_offset[0][eADS].z;
	if (scope && pSettings->line_exist(section, "scope_cam_z_offset"))
		m_hands_offset[0][eADS].z		+= pSettings->r_float(section, "scope_cam_z_offset");
	else if (pSettings->line_exist(section, "cam_z_offset"))
		m_hands_offset[0][eADS].z		= pSettings->r_float(section, "cam_z_offset");

	m_hands_offset[0][eAim].lerp		(m_hands_offset[0][eArmed], m_hands_offset[0][eADS], aim_factor);
	m_hands_offset[1][eAim].lerp		(m_hands_offset[1][eArmed], m_hands_offset[1][eADS], aim_factor);

	m_hands_offset[0][eGL]			= pSettings->r_fvector3(section, "gl_sights_pos");
	m_hands_offset[1][eGL]			= pSettings->r_fvector3(section, "gl_sights_rot");

	////////////////////////////////////////////
	//--#SM+# Begin--

	// Альтернативное прицеливание
	if (scope && pSettings->r_bool(section, "scope_alt_aim_via_iron_sights"))
	{
		m_hands_offset[0][eAlt]			= pSettings->r_fvector3(section, "iron_sights_pos");
		m_hands_offset[1][eAlt]			= pSettings->r_fvector3(section, "iron_sights_rot");
	}
	else if (scope && pSettings->line_exist(scope, "alt_aim_pos"))
	{
		m_hands_offset[0][eAlt]			= m_hands_offset[0][eADS];
		m_hands_offset[0][eAlt].add		(pSettings->r_fvector3(scope, "alt_aim_pos"));
		m_hands_offset[1][eAlt]			= m_hands_offset[1][eADS];
		m_hands_offset[1][eAlt].add		(pSettings->r_fvector3(scope, "alt_aim_rot"));
	}
	else
	{
		m_hands_offset[0][eAlt]			= pSettings->r_fvector3(section, "alt_aim_pos");
		m_hands_offset[1][eAlt]			= pSettings->r_fvector3(section, "alt_aim_rot");
		m_hands_offset[0][eAlt].y		-= pSettings->r_float(section, "alt_aim_height");
	}
	
	m_hands_offset[0][eGL].z			= m_hands_offset[0][eADS].z;
	m_hands_offset[0][eAlt].z			= m_hands_offset[0][eADS].z;

	// Safemode / lowered weapon position
	m_hands_offset[0][eRelaxed]			= pSettings->r_fvector3(section, "relaxed_pos");
	m_hands_offset[1][eRelaxed]			= pSettings->r_fvector3(section, "relaxed_rot");

	// Загрузка параметров смещения при стрельбе
	m_shooting_params.bShootShake = READ_IF_EXISTS(pSettings, r_bool, section, "shooting_hud_effect", false);
	m_shooting_params.m_shot_max_offset_LRUD = READ_IF_EXISTS(pSettings, r_fvector4, section, "shooting_max_LRUD", Fvector4().set(0, 0, 0, 0));
	m_shooting_params.m_shot_max_offset_LRUD_aim = READ_IF_EXISTS(pSettings, r_fvector4, section, "shooting_max_LRUD_aim", Fvector4().set(0, 0, 0, 0));
	m_shooting_params.m_shot_offset_BACKW = READ_IF_EXISTS(pSettings, r_fvector2, section, "shooting_backward_offset", Fvector2().set(0, 0));
	m_shooting_params.m_ret_speed = READ_IF_EXISTS(pSettings, r_float, section, "shooting_ret_speed", 1.0f);
	m_shooting_params.m_ret_speed_aim = READ_IF_EXISTS(pSettings, r_float, section, "shooting_ret_aim_speed", 1.0f);
	m_shooting_params.m_min_LRUD_power = READ_IF_EXISTS(pSettings, r_float, section, "shooting_min_LRUD_power", 0.0f);

	//--#SM+# End--

	for (int i = 0; i < 2; i++)
		m_hud_offset[i] = vZero;

	m_fRotationFactor = 0.f;

	m_fLR_ShootingFactor = 0.f;
	m_fUD_ShootingFactor = 0.f;
	m_fBACKW_ShootingFactor = 0.f;

	m_shoot_shake_mat.identity();
}

bool CWeaponHud::IsRotatingToZoom C$()
{
	return !fEqual(m_fRotationFactor, 1.f, EPS_S) && !fEqual(m_fRotationFactor, 0.f, EPS_S);
}

bool CWeaponHud::ReadyToFire C$()
{
	return (GetCurrentHudOffsetIdx() != eRelaxed && last_idx != eRelaxed);
}

void CWeaponHud::InitRotateTime(float cif)
{
	float inertion = cif - 1.f;
	m_fRotateTime = HandlingToRotationTime.Calc(inertion);
	m_fRelaxTime = HandlingToRelaxTime.Calc(inertion);
}

EHandsOffset CWeaponHud::GetCurrentHudOffsetIdx C$()
{
	if (HUD().GetCurrentRayQuery().range < 1.f && !smart_cast<CEntityAlive*>(Actor()->ObjectWeLookingAt()))
		return eRelaxed;

	if (o().IsZoomed())
	{
		switch (o().ADS())
		{
		case 0:
			return eAim;
		case 1:
			return eADS;
		case -1:
			return eAlt;
		case 2:
			return eGL;
		}
	}

	if (!o().ArmedMode())
	{
		if (o().HandSlot() == BOTH_HANDS_SLOT)
			return eRelaxed;

		CEntity::SEntityState st;
		smart_cast<CActor*>(o().H_Parent())->g_State(st);
		if (!st.bSprint)
			return eRelaxed;
	}

	return eArmed;
}

Fvector cur_offs = Fvector().set(0.f, 0.f, 0.f);
void CWeaponHud::UpdateHudAdditional(Fmatrix& trans)
{
	attachable_hud_item* hi = o().HudItemData();
	R_ASSERT(hi);

	EHandsOffset idx = GetCurrentHudOffsetIdx();
	
	//============= Поворот ствола во время аима =============//
	{
		Fvector CR$ curr_offs = m_hands_offset[0][idx]; //pos,aim
		Fvector CR$ curr_rot = m_hands_offset[1][idx]; //rot,aim

		float factor = Device.fTimeDelta;
		if (idx == eRelaxed || last_idx == eRelaxed)
			factor /= m_fRelaxTime;
		else
			factor /= m_fRotateTime;

		if (curr_offs.similar(m_hud_offset[0], EPS))
			m_hud_offset[0].set(curr_offs);
		else
		{
			Fvector diff = curr_offs;
			diff.sub(m_hud_offset[0]);
			diff.mul(factor * 2.5f);
			m_hud_offset[0].add(diff);
		}

		if (curr_rot.similar(m_hud_offset[1], EPS))
			m_hud_offset[1].set(curr_rot);
		else
		{
			Fvector diff = curr_rot;
			diff.sub(m_hud_offset[1]);
			diff.mul(factor * 2.5f);
			m_hud_offset[1].add(diff);
		}

		// Remove pending state before weapon has fully moved to the new position to remove some delay
		if (curr_offs.similar(m_hud_offset[0], .02f) && curr_rot.similar(m_hud_offset[1], .02f))
		{
			if ((idx == eRelaxed || last_idx == eRelaxed) && o().IsPending())
				o().SetPending(FALSE);
			last_idx = idx;
		}

		Fmatrix hud_rotation;
		hud_rotation.identity();
		hud_rotation.rotateX(m_hud_offset[1].x);

		Fmatrix hud_rotation_y;
		hud_rotation_y.identity();
		hud_rotation_y.rotateY(m_hud_offset[1].y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(m_hud_offset[1].z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(m_hud_offset[0]);
		trans.mulB_43(hud_rotation);

		if (o().IsZoomed())
			m_fRotationFactor += factor;
		else
			m_fRotationFactor -= factor;

		clamp(m_fRotationFactor, 0.f, 1.f);
	}
	
	//============= Подготавливаем общие переменные =============//
	bool bForAim = (idx >= eAim);

	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

	//============= Сдвиг оружия при стрельбе =============//
	if (m_shooting_params.bShootShake)
	{
		// Параметры сдвига
		float fShootingReturnSpeedMod = _lerp(
			m_shooting_params.m_ret_speed,
			m_shooting_params.m_ret_speed_aim,
			m_fRotationFactor);

		float fShootingBackwOffset = _lerp(
			m_shooting_params.m_shot_offset_BACKW.x,
			m_shooting_params.m_shot_offset_BACKW.y,
			m_fRotationFactor);

		Fvector4 vShOffsets; // x = L, y = R, z = U, w = D
		vShOffsets.x = _lerp(
			m_shooting_params.m_shot_max_offset_LRUD.x,
			m_shooting_params.m_shot_max_offset_LRUD_aim.x,
			m_fRotationFactor);
		vShOffsets.y = _lerp(
			m_shooting_params.m_shot_max_offset_LRUD.y,
			m_shooting_params.m_shot_max_offset_LRUD_aim.y,
			m_fRotationFactor);
		vShOffsets.z = _lerp(
			m_shooting_params.m_shot_max_offset_LRUD.z,
			m_shooting_params.m_shot_max_offset_LRUD_aim.z,
			m_fRotationFactor);
		vShOffsets.w = _lerp(
			m_shooting_params.m_shot_max_offset_LRUD.w,
			m_shooting_params.m_shot_max_offset_LRUD_aim.w,
			m_fRotationFactor);

		// Плавное затухание сдвига от стрельбы (основное, но без линейной никогда не опустит до полного 0.0f)
		m_fLR_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
		m_fUD_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
		m_fBACKW_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);

		// Минимальное линейное затухание сдвига от стрельбы при покое (горизонталь)
		{
			float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
			if (m_fLR_ShootingFactor < 0.0f)
			{
				m_fLR_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
				clamp(m_fLR_ShootingFactor, -1.0f, 0.0f);
			}
			else
			{
				m_fLR_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
				clamp(m_fLR_ShootingFactor, 0.0f, 1.0f);
			}
		}

		// Минимальное линейное затухание сдвига от стрельбы при покое (вертикаль)
		{
			float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
			if (m_fUD_ShootingFactor < 0.0f)
			{
				m_fUD_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
				clamp(m_fUD_ShootingFactor, -1.0f, 0.0f);
			}
			else
			{
				m_fUD_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
				clamp(m_fUD_ShootingFactor, 0.0f, 1.0f);
			}
		}

		// Минимальное линейное затухание сдвига от стрельбы при покое (вперёд\назад)
		{
			float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
			m_fBACKW_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fBACKW_ShootingFactor, 0.0f, 1.0f);
		}

		// Применяем сдвиг от стрельбы к худу
		{
			float fLR_lim = (m_fLR_ShootingFactor < 0.0f ? vShOffsets.x : vShOffsets.y);
			float fUD_lim = (m_fUD_ShootingFactor < 0.0f ? vShOffsets.z : vShOffsets.w);

			Fvector cur_offs = {
				fLR_lim * m_fLR_ShootingFactor,
				fUD_lim * -1.f * m_fUD_ShootingFactor,
				-1.f * fShootingBackwOffset * m_fBACKW_ShootingFactor
			};

			m_shoot_shake_mat.translate_over(cur_offs);
			trans.mulB_43(m_shoot_shake_mat);
		}
	}

	//======== Проверяем доступность инерции и стрейфа ========//
	if (!g_player_hud->inertion_allowed())
		return;

	float fYMag = Actor()->fFPCamYawMagnitude;
	float fPMag = Actor()->fFPCamPitchMagnitude;

	//============= Боковой стрейф с оружием =============//
	// Рассчитываем фактор боковой ходьбы
	hud_item_measures CR$ m = o().HudItemData()->m_measures;
	float fStrafeMaxTime = m.m_strafe_offset[2][idx].y;
	// Макс. время в секундах, за которое мы наклонимся из центрального положения
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

	// Добавляем боковой наклон от движения камеры
	float fCamReturnSpeedMod = 1.5f;
	// Восколько ускоряем нормализацию наклона, полученного от движения камеры (только от бедра)

	// Высчитываем минимальную скорость поворота камеры для начала инерции
	float fStrafeMinAngle = _lerp(
		m.m_strafe_offset[3][0].y,
		m.m_strafe_offset[3][1].y,
		m_fRotationFactor);

	// Высчитываем мксимальный наклон от поворота камеры
	float fCamLimitBlend = _lerp(
		m.m_strafe_offset[3][0].x,
		m.m_strafe_offset[3][1].x,
		m_fRotationFactor);

	// Считаем стрейф от поворота камеры
	if (abs(fYMag) > (o().m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{
		//--> Камера крутится по оси Y
		o().m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
		clamp(o().m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{
		//--> Камера не поворачивается - убираем наклон
		if (o().m_fLR_CameraFactor < 0.0f)
		{
			o().m_fLR_CameraFactor += fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(o().m_fLR_CameraFactor, -fCamLimitBlend, 0.0f);
		}
		else
		{
			o().m_fLR_CameraFactor -= fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(o().m_fLR_CameraFactor, 0.0f, fCamLimitBlend);
		}
	}

	// Добавляем боковой наклон от ходьбы вбок
	float fChangeDirSpeedMod = 3;
	// Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего
	u32 iMovingState = Actor()->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{
		// Движемся влево
		float fVal = (o().m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		o().m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{
		// Движемся вправо
		float fVal = (o().m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		o().m_fLR_MovingFactor += fVal;
	}
	else
	{
		// Двигаемся в любом другом направлении - плавно убираем наклон
		if (o().m_fLR_MovingFactor < 0.0f)
		{
			o().m_fLR_MovingFactor += fStepPerUpd;
			clamp(o().m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			o().m_fLR_MovingFactor -= fStepPerUpd;
			clamp(o().m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}
	clamp(o().m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Вычисляем и нормализируем итоговый фактор наклона
	float fLR_Factor = o().m_fLR_MovingFactor; 
	fLR_Factor += o().m_fLR_CameraFactor;

	clamp(fLR_Factor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Производим наклон ствола для нормального режима и аима
	for (int _idx = 0; _idx <= 1; _idx++) //<-- Для плавного перехода
	{
		bool bEnabled = (m.m_strafe_offset[2][_idx].x != 0.0f);
		if (!bEnabled)
			continue;

		Fvector curr_offs, curr_rot;

		// Смещение позиции худа в стрейфе
		curr_offs = m.m_strafe_offset[0][_idx]; // pos
		curr_offs.mul(fLR_Factor); // Умножаем на фактор стрейфа

		// Поворот худа в стрейфе
		curr_rot = m.m_strafe_offset[1][_idx]; // rot
		curr_rot.mul(-PI / 180.f); // Преобразуем углы в радианы
		curr_rot.mul(fLR_Factor); // Умножаем на фактор стрейфа

		// Мягкий переход между бедром \ прицелом
		if (_idx == 0)
		{
			// От бедра
			curr_offs.mul(1.f - m_fRotationFactor);
			curr_rot.mul(1.f - m_fRotationFactor);
		}
		else
		{
			// Во время аима
			curr_offs.mul(m_fRotationFactor);
			curr_rot.mul(m_fRotationFactor);
		}

		Fmatrix hud_rotation;
		Fmatrix hud_rotation_y;

		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		trans.mulB_43(hud_rotation);
	}

	//============= Инерция оружия =============//
	// Параметры инерции
	float fInertiaSpeedMod = _lerp(
		m.m_inertion_params.m_tendto_speed,
		m.m_inertion_params.m_tendto_speed_aim,
		m_fRotationFactor);

	float fInertiaReturnSpeedMod = _lerp(
		m.m_inertion_params.m_tendto_ret_speed,
		m.m_inertion_params.m_tendto_ret_speed_aim,
		m_fRotationFactor);

	float fInertiaMinAngle = _lerp(
		m.m_inertion_params.m_min_angle,
		m.m_inertion_params.m_min_angle_aim,
		m_fRotationFactor);

	Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
	vIOffsets.x = _lerp(
		m.m_inertion_params.m_offset_LRUD.x,
		m.m_inertion_params.m_offset_LRUD_aim.x,
		m_fRotationFactor);
	vIOffsets.y = _lerp(
		m.m_inertion_params.m_offset_LRUD.y,
		m.m_inertion_params.m_offset_LRUD_aim.y,
		m_fRotationFactor);
	vIOffsets.z = _lerp(
		m.m_inertion_params.m_offset_LRUD.z,
		m.m_inertion_params.m_offset_LRUD_aim.z,
		m_fRotationFactor);
	vIOffsets.w = _lerp(
		m.m_inertion_params.m_offset_LRUD.w,
		m.m_inertion_params.m_offset_LRUD_aim.w,
		m_fRotationFactor);

	// Высчитываем инерцию из поворотов камеры
	bool bIsInertionPresent = !fIsZero(o().m_fLR_InertiaFactor) || !fIsZero(o().m_fUD_InertiaFactor);
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && o().m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && o().m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		o().m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Горизонталь (м.б. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && o().m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && o().m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		o().m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Вертикаль (м.б. > |1.0|)
	}

	clamp(o().m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(o().m_fUD_InertiaFactor, -1.0f, 1.0f);

	// Плавное затухание инерции (основное, но без линейной никогда не опустит инерцию до полного 0.0f)
	o().m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	o().m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// Минимальное линейное затухание инерции при покое (горизонталь)
	if (fYMag == 0.0f)
	{
		float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (o().m_fLR_InertiaFactor < 0.0f)
		{
			o().m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(o().m_fLR_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			o().m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(o().m_fLR_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Минимальное линейное затухание инерции при покое (вертикаль)
	if (fPMag == 0.0f)
	{
		float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (o().m_fUD_InertiaFactor < 0.0f)
		{
			o().m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(o().m_fUD_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			o().m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(o().m_fUD_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Применяем инерцию к худу
	float fLR_lim = (o().m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (o().m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	cur_offs = { fLR_lim * -1.f * o().m_fLR_InertiaFactor, fUD_lim * o().m_fUD_InertiaFactor, 0.0f };

	Fmatrix hud_rotation;
	hud_rotation.identity();
	hud_rotation.translate_over(cur_offs);
	trans.mulB_43(hud_rotation);
}

extern BOOL								g_hud_adjusment_mode;
int hands_mode							= 0;
bool CWeaponHud::Action(u16 cmd, u32 flags)
{
	if (g_hud_adjusment_mode && flags&CMD_START)
	{
		int axis						= -1;
		switch (cmd)
		{
		case kL_STRAFE:
		case kR_STRAFE:
		case kUP:
		case kDOWN:
			axis						= 0;
			break;
		case kFWD:
		case kBACK:
		case kLEFT:
		case kRIGHT:
			axis						= 1;
			break;
		case kL_LOOKOUT:
		case kR_LOOKOUT:
			axis						= 2;
			break;
		case kJUMP:
			if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
				hands_mode				= int(!hands_mode);
			else if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
				hands_mode				= 2;
			else
			{
				std::ofstream			out;
				out.open				("C:\\tmp.txt");
				Fvector CP$ vp			= &o().HudItemData()->m_measures.m_hands_attach[0];
				Fvector CP$ vr			= &o().HudItemData()->m_measures.m_hands_attach[1];
				out						<< shared_str().printf("m_hands_attach pos [%.6f,%.6f,%.6f] m_hands_attach rot [%.6f,%.6f,%.6f]", vp->x, vp->y, vp->z, vr->x, vr->y, vr->z).c_str() << std::endl;
				vp						= &m_hands_offset[0][GetCurrentHudOffsetIdx()];
				vr						= &m_hands_offset[1][GetCurrentHudOffsetIdx()];
				out						<< shared_str().printf("m_hands_offset pos [%.6f,%.6f,%.6f] m_hands_offset rot [%.6f,%.6f,%.6f]", vp->x, vp->y, vp->z, vr->x, vr->y, vr->z).c_str() << std::endl;
			}
			return						true;
		}

		if (axis != -1)
		{
			float val					= 0.0001f;
			if (cmd == kL_STRAFE || cmd == kLEFT || cmd == kBACK || cmd == kUP || cmd == kL_LOOKOUT)
				val						*= -1.f;
			if (pInput->iGetAsyncKeyState(DIK_LSHIFT))
				val						*= pInput->iGetAsyncKeyState(DIK_LALT) ? .1f : 100.f;
			if (pInput->iGetAsyncKeyState(DIK_LCONTROL))
				val						*= 10.f;
			if (pInput->iGetAsyncKeyState(DIK_LALT))
				val						/= 10.f;

			BOOL rot					= BOOL(cmd == kUP || cmd == kDOWN || cmd == kLEFT || cmd == kRIGHT || pInput->iGetAsyncKeyState(DIK_RCONTROL) && axis == 2);

			if (hands_mode == 1 && rot || hands_mode == 2 && !rot)
			{
				if (rot && axis != 2)
				{
					axis				= !axis;
					val					*= -1.f;
				}
				o().HudItemData()->m_measures.m_hands_attach[rot][axis] += val;
			}
			else
				m_hands_offset[rot][GetCurrentHudOffsetIdx()][axis] += val;
			return						true;
		}
	}

	return false;
}
