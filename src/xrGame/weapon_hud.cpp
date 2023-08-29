#include "stdafx.h"
#include "weapon_hud.h"
#include "WeaponMagazined.h"
#include "HUDManager.h"
#include "xr_level_controller.h"
#include "../../xrEngine/xr_input.h"
#include <fstream>
#include "player_hud.h"
#include "Level_Bullet_Manager.h"
#include "addon.h"
#include "scope.h"
#include "weapon.h"

const float aim_factor = pSettings->r_float("weapon_manager", "aim_factor");

void ApplyPivot(Fvector* offset, Fvector CR$ pivot)
{
	offset[0].add						(pivot.rotate_(offset[1]));
}

void ApplyOffset(Fmatrix& trans, Fvector CR$ position, Fvector CR$ rotation)
{
	Fmatrix								hud_rotation;
	hud_rotation.identity				();
	hud_rotation.rotateX				(rotation.x);

	Fmatrix								hud_rotation_y;
	hud_rotation_y.identity				();
	hud_rotation_y.rotateY				(rotation.y);
	hud_rotation.mulA_43				(hud_rotation_y);

	hud_rotation_y.identity				();
	hud_rotation_y.rotateZ				(rotation.z);
	hud_rotation.mulA_43				(hud_rotation_y);

	hud_rotation.translate_over			(position);
	trans.mulB_43						(hud_rotation);
}

CWeaponHud::CWeaponHud(CWeaponMagazined* obj) : O(*obj)
{
	m_fRotationFactor					= 0.f;
	m_fLR_ShootingFactor				= 0.f;
	m_fUD_ShootingFactor				= 0.f;
	m_fBACKW_ShootingFactor				= 0.f;
	m_shoot_shake_mat.identity			();
	m_lense_offset						= 0.f;
	m_scope								= 0;
	m_alt_scope							= false;
	m_gl								= false;

	for (int i = 0; i < 2; i++)
		m_hud_offset[i]					= vZero;

	m_root_offset						= pSettings->r_fvector3(O.HudSection(), "root_offset");

	m_hands_offset[eIS][0]				= vZero;
	m_hands_offset[eIS][0].sub			(pSettings->r_fvector3(O.HudSection(), "iron_sights_pos"));
	m_hands_offset[eIS][1]				= vZero;
	m_hands_offset[eIS][1].sub			(pSettings->r_fvector3(O.HudSection(), "iron_sights_rot"));
	ApplyPivot							(m_hands_offset[eIS], m_root_offset);

	m_barrel_offset						= m_root_offset;
	m_barrel_offset.sub					(pSettings->r_fvector3(O.HudSection(), "barrel_position"));

	m_hands_offset[eRelaxed][0]			= pSettings->r_fvector3(O.HudSection(), "relaxed_pos");
	m_hands_offset[eRelaxed][1]			= pSettings->r_fvector3(O.HudSection(), "relaxed_rot");
	ApplyPivot							(m_hands_offset[eRelaxed], m_barrel_offset);

	m_hands_offset[eArmed][0]			= pSettings->r_fvector3(O.HudSection(), "armed_pos");
	m_hands_offset[eArmed][1]			= pSettings->r_fvector3(O.HudSection(), "armed_rot");
	ApplyPivot							(m_hands_offset[eArmed], m_barrel_offset);

	m_hands_offset[eAlt][0]				= pSettings->r_fvector3(O.HudSection(), "alt_aim_pos");
	m_hands_offset[eAlt][1]				= pSettings->r_fvector3(O.HudSection(), "alt_aim_rot");
	ApplyPivot							(m_hands_offset[eAlt], m_barrel_offset);
	m_hands_offset[eAlt][0].y			-= pSettings->r_float(O.HudSection(), "alt_aim_height");

	m_hands_offset[eIS][0].z			= m_barrel_offset.z + pSettings->r_float(O.HudSection(), "cam_z_offset");
	m_hands_offset[eAlt][0].z			= m_hands_offset[eIS][0].z;

	CalcAimOffset						();

	// Загрузка параметров смещения при стрельбе
	m_shooting_params.bShootShake		= READ_IF_EXISTS(pSettings, r_bool, O.HudSection(), "shooting_hud_effect", false);
	m_shooting_params.m_shot_max_offset_LRUD = READ_IF_EXISTS(pSettings, r_fvector4, O.HudSection(), "shooting_max_LRUD", Fvector4().set(0, 0, 0, 0));
	m_shooting_params.m_shot_max_offset_LRUD_aim = READ_IF_EXISTS(pSettings, r_fvector4, O.HudSection(), "shooting_max_LRUD_aim", Fvector4().set(0, 0, 0, 0));
	m_shooting_params.m_shot_offset_BACKW = READ_IF_EXISTS(pSettings, r_fvector2, O.HudSection(), "shooting_backward_offset", Fvector2().set(0, 0));
	m_shooting_params.m_ret_speed		= READ_IF_EXISTS(pSettings, r_float, O.HudSection(), "shooting_ret_speed", 1.0f);
	m_shooting_params.m_ret_speed_aim	= READ_IF_EXISTS(pSettings, r_float, O.HudSection(), "shooting_ret_aim_speed", 1.0f);
	m_shooting_params.m_min_LRUD_power	= READ_IF_EXISTS(pSettings, r_float, O.HudSection(), "shooting_min_LRUD_power", 0.0f);

	//--#SM+# End--

	m_scope_alt_aim_via_iron_sights		= pSettings->r_bool(O.HudSection(), "scope_alt_aim_via_iron_sights");
	m_scope_own_alt_aim					= false;

	m_going_to_fire						= false;
}

void CWeaponHud::CalcAimOffset()
{
	EHandsOffset idx					= (m_gl) ? eGL : ((m_scope) ? eScope : eIS);
	m_hands_offset[eAim][0].lerp		(m_hands_offset[eArmed][0], m_hands_offset[idx][0], aim_factor);
	m_hands_offset[eAim][1].lerp		(m_hands_offset[eArmed][1], m_hands_offset[idx][1], aim_factor);
}

void CWeaponHud::ProcessScope(SAddonSlot* slot, bool attach)
{
	if (slot->alt_scope)
		m_alt_scope						= attach;
	else
		m_scope							= (attach) ? slot->addon->cast<CScope*>()->Type() : 0;
	if (!attach)
		return;

	EHandsOffset idx					= (slot->alt_scope) ? eScopeAlt : eScope;

	m_hands_offset[idx][0]				= vZero;
	m_hands_offset[idx][1]				= vZero;
	m_hands_offset[idx][1].sub			(slot->model_offset[1]);

	Fvector tmp							= m_root_offset;
	tmp.sub								(slot->model_offset[0]);
	ApplyPivot							(m_hands_offset[idx], tmp);

	if (idx == eScope)
	{
		if (!m_alt_scope && pSettings->line_exist(slot->addon->cNameSect(), "alt_aim_pos"))
		{
			m_hands_offset[eScopeAlt][0] = m_hands_offset[eScope][0];
			m_hands_offset[eScopeAlt][1] = m_hands_offset[eScope][1];
			m_hands_offset[eScopeAlt][0].sub(pSettings->r_fvector3(slot->addon->cNameSect(), "alt_aim_pos"));
			m_scope_own_alt_aim			= true;
		}
		else
			m_scope_own_alt_aim			= false;
	}

	m_hands_offset[idx][0].add			(slot->addon->HudOffset()[0]);

	if (idx == eScope)
	{
		m_lense_offset					= m_hands_offset[eScope][0].z;
		m_hands_offset[eScope][0].z		= m_hands_offset[eIS][0].z;
		CalcAimOffset					();
	}
	m_hands_offset[eScopeAlt][0].z		= m_hands_offset[eIS][0].z;
}

void CWeaponHud::ProcessGL(SAddonSlot* slot, bool attach)
{
	m_root_offset_gl					= READ_IF_EXISTS(pSettings, r_fvector3, O.HudSection(), "root_offset_alt", m_root_offset);

	Fvector tmp							= m_root_offset_gl;
	tmp.sub								(slot->model_offset[0]);

	m_hands_offset[eGL][0]				= slot->addon->HudOffset()[0];
	m_hands_offset[eGL][1]				= slot->addon->HudOffset()[1];
	ApplyPivot							(m_hands_offset[eGL], tmp);

	m_hands_offset[eGL][0].z			= m_hands_offset[eIS][0].z;
}

void CWeaponHud::SwitchGL()
{
	m_gl								= !m_gl;
	CalcAimOffset						();
}

bool CWeaponHud::IsRotatingToZoom C$()
{
	EHandsOffset idx = GetCurrentHudOffsetIdx();
	static const float eps = .01f;

	if (!m_hands_offset[idx][0].similar(m_hud_offset[0], eps))
		return true;

	if (!m_hands_offset[idx][1].similar(m_hud_offset[1], eps))
		return true;

	return false;
}

void CWeaponHud::InitRotateTime(float cif)
{
	m_fRotateTime = HandlingToRotationTime.Calc(cif - 1.f);
}

EHandsOffset CWeaponHud::GetCurrentHudOffsetIdx() const
{
	if (HUD().GetCurrentRayQuery().range < .7f && !smart_cast<CEntityAlive*>(Actor()->ObjectWeLookingAt()))
		return eRelaxed;

	if (O.IsZoomed())
	{
		switch (O.ADS())
		{
		case 0:
			return eAim;
		case 1:
			return (m_scope) ? eScope : eIS;
		case 2:
			return eGL;
		case -1:
			return (m_alt_scope || m_scope_own_alt_aim) ? eScopeAlt : ((m_scope && m_scope_alt_aim_via_iron_sights) ? eIS : eAlt);
		}
	}

	if (O.Cast<CHudItem*>()->GetState() == CWeapon::eReload)
		return eArmed;

	if (!O.ArmedMode())
	{
		if (O.HandSlot() == BOTH_HANDS_SLOT)
			return eRelaxed;

		CEntity::SEntityState st;
		Actor()->g_State(st);
		if (!st.bSprint)
			return eRelaxed;
	}

	return eArmed;
}

void CWeaponHud::UpdateHudAdditional(Fmatrix& trans)
{
	EHandsOffset idx = GetCurrentHudOffsetIdx();
	
	//============= Поворот ствола во время аима =============//
	{
		Fvector CR$ curr_offs = m_hands_offset[idx][0]; //pos,aim
		Fvector CR$ curr_rot = m_hands_offset[idx][1]; //rot,aim

		float factor = Device.fTimeDelta / m_fRotateTime;
		switch (idx)
		{
			case eRelaxed:
			case eIS:
			case eAlt:
			case eScopeAlt:
			case eGL:
				factor /= .8f;
				break;
			case eArmed:
			case eAim:
				factor /= .4f;
				break;
			case eScope:
				factor /= (m_scope == eOptics) ? 1.f : .6f;
				break;
		}

		if (curr_offs.similar(m_hud_offset[0], EPS_S))
			m_hud_offset[0].set(curr_offs);
		else
		{
			Fvector diff = Fvector(curr_offs).sub(m_hud_offset[0]);
			m_hud_offset[0].mad(diff, factor * 2.5f);
		}

		if (curr_rot.similar(m_hud_offset[1], EPS_S))
			m_hud_offset[1].set(curr_rot);
		else
		{
			Fvector diff = Fvector(curr_rot).sub(m_hud_offset[1]);
			m_hud_offset[1].mad(diff, factor * 2.5f);
		}

		// Remove pending state before weapon has fully moved to the new position to remove some delay
		if (m_going_to_fire && curr_offs.similar(m_hud_offset[0], .01f) && curr_rot.similar(m_hud_offset[1], .01f))
		{
			O.FireStart();
			m_going_to_fire = false;
		}

		ApplyOffset(trans, m_hud_offset[0], m_hud_offset[1]);

		if (O.IsZoomed())
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
	hud_item_measures CR$ m = O.HudItemData()->m_measures;
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
	if (abs(fYMag) > (O.m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{
		//--> Камера крутится по оси Y
		O.m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
		clamp(O.m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{
		//--> Камера не поворачивается - убираем наклон
		if (O.m_fLR_CameraFactor < 0.0f)
		{
			O.m_fLR_CameraFactor += fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(O.m_fLR_CameraFactor, -fCamLimitBlend, 0.0f);
		}
		else
		{
			O.m_fLR_CameraFactor -= fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
			clamp(O.m_fLR_CameraFactor, 0.0f, fCamLimitBlend);
		}
	}

	// Добавляем боковой наклон от ходьбы вбок
	float fChangeDirSpeedMod = 3;
	// Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего
	u32 iMovingState = Actor()->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{
		// Движемся влево
		float fVal = (O.m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		O.m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{
		// Движемся вправо
		float fVal = (O.m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		O.m_fLR_MovingFactor += fVal;
	}
	else
	{
		// Двигаемся в любом другом направлении - плавно убираем наклон
		if (O.m_fLR_MovingFactor < 0.0f)
		{
			O.m_fLR_MovingFactor += fStepPerUpd;
			clamp(O.m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			O.m_fLR_MovingFactor -= fStepPerUpd;
			clamp(O.m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}
	clamp(O.m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Вычисляем и нормализируем итоговый фактор наклона
	float fLR_Factor = O.m_fLR_MovingFactor; 
	fLR_Factor += O.m_fLR_CameraFactor;

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

		ApplyOffset(trans, curr_offs, curr_rot);
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
	bool bIsInertionPresent = !fIsZero(O.m_fLR_InertiaFactor) || !fIsZero(O.m_fUD_InertiaFactor);
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && O.m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && O.m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		O.m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Горизонталь (м.б. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && O.m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && O.m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		O.m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Вертикаль (м.б. > |1.0|)
	}

	clamp(O.m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(O.m_fUD_InertiaFactor, -1.0f, 1.0f);

	// Плавное затухание инерции (основное, но без линейной никогда не опустит инерцию до полного 0.0f)
	O.m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	O.m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// Минимальное линейное затухание инерции при покое (горизонталь)
	if (fYMag == 0.0f)
	{
		float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (O.m_fLR_InertiaFactor < 0.0f)
		{
			O.m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(O.m_fLR_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			O.m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(O.m_fLR_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Минимальное линейное затухание инерции при покое (вертикаль)
	if (fPMag == 0.0f)
	{
		float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (O.m_fUD_InertiaFactor < 0.0f)
		{
			O.m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(O.m_fUD_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			O.m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(O.m_fUD_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Применяем инерцию к худу
	float fLR_lim = (O.m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (O.m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	Fvector cur_offs = { fLR_lim * -1.f * O.m_fLR_InertiaFactor, fUD_lim * O.m_fUD_InertiaFactor, 0.0f };
	ApplyOffset(trans, cur_offs, vZero);
}

extern BOOL								g_hud_adjusment_mode;
int hands_mode							= 0;
bool CWeaponHud::Action(u16 cmd, u32 flags)
{
	if (cmd == kWPN_FIRE)
	{
		if (flags&CMD_START)
		{
			if (GetCurrentHudOffsetIdx() == eRelaxed)
			{
				if (!O.ArmedMode() && ! O.IsZoomed())
				{
					O.SwitchArmedMode	();
					m_going_to_fire		= true;
				}
				return					true;
			}
		}
		else if (m_going_to_fire)
			m_going_to_fire				= false;
	}

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
				Fvector vp				= O.HudItemData()->m_measures.m_hands_attach[0];
				Fvector CP$ vr			= &O.HudItemData()->m_measures.m_hands_attach[(m_gl) ? 2 : 1];
				out						<< shared_str().printf("m_hands_attach pos [%.6f,%.6f,%.6f] m_hands_attach rot [%.6f,%.6f,%.6f]", vp.x, vp.y, vp.z, vr->x, vr->y, vr->z).c_str() << std::endl;
				vp						= m_hands_offset[GetCurrentHudOffsetIdx()][0];
				vr						= &m_hands_offset[GetCurrentHudOffsetIdx()][1];
				out						<< shared_str().printf("m_hands_offset pos [%.6f,%.6f,%.6f] m_hands_offset rot [%.6f,%.6f,%.6f]", vp.x, vp.y, vp.z, vr->x, vr->y, vr->z).c_str() << std::endl;
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
			if (m_gl && rot)
				rot						= 2;

			if (hands_mode == 1 && rot || hands_mode == 2 && !rot)
			{
				if (rot && axis != 2)
				{
					axis				= !axis;
					val					*= -1.f;
				}
				O.HudItemData()->m_measures.m_hands_attach[rot][axis] += val;
			}
			else
				m_hands_offset[GetCurrentHudOffsetIdx()][rot][axis] += val;
			return						true;
		}
	}

	return false;
}

Fvector CWeaponHud::BarrelSightOffset C$()
{
	return								Fvector(m_barrel_offset).sub(m_hands_offset[GetCurrentHudOffsetIdx()][0]);
}
