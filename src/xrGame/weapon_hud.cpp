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

CWeaponHud::CWeaponHud(CWeaponMagazined* obj) : O(*obj)
{
	for (int i = 0; i < 2; i++)
		m_current_hud_offset[i]			= vZero;

	m_hud_offset[eRelaxed][0]			= pSettings->r_fvector3(O.HudSection(), "relaxed_pos");
	m_hud_offset[eRelaxed][1]			= pSettings->r_fvector3d2r(O.HudSection(), "relaxed_rot");
	O.m_grip_offset.pivot				(m_hud_offset[eRelaxed]);

	m_hud_offset[eArmed][0]				= pSettings->r_fvector3(O.HudSection(), "armed_pos");
	m_hud_offset[eArmed][1]				= pSettings->r_fvector3d2r(O.HudSection(), "armed_rot");
	O.m_grip_offset.pivot				(m_hud_offset[eArmed]);

	m_hud_offset[eIS][0]				= O.m_root_bone_position;
	m_hud_offset[eIS][0].sub			(pSettings->r_fvector3(O.Section(), "iron_sights_pos"));
	m_hud_offset[eIS][0].z				+= pSettings->r_float(O.Section(), "cam_z_offset");
	m_hud_offset[eIS][1]				= vZero;

	auto barrel_pos						= vZero;
	barrel_pos.sub						(O.m_muzzle_point);
	float aim_height					= pSettings->r_float(O.Section(), "alt_aim_height");
	
	m_hud_offset[eAlt][0]				= pSettings->r_fvector3(O.HudSection(), "alt_aim_pos");
	m_hud_offset[eAlt][1]				= pSettings->r_fvector3(O.HudSection(), "alt_aim_rot");
	barrel_pos.pivot					(m_hud_offset[eAlt]);
	m_hud_offset[eAlt][0].y				-= aim_height;
	m_hud_offset[eAlt][0].z				= m_hud_offset[eIS][0].z;
	
	m_hud_offset[eAim][1]				= vZero;
	m_hud_offset[eAim][0]				= pSettings->r_fvector3(O.HudSection(), "aim_pos");
	Fvector dir							= m_hud_offset[eAim][0];
	dir.z								= 0.f;
	dir.normalize						();
	m_hud_offset[eAim][0].add			(barrel_pos);
	m_hud_offset[eAim][0].z				= m_hud_offset[eIS][0].z;
	m_hud_offset[eAim][0].mad			(dir, aim_height);
}

void CWeaponHud::ProcessScope(CScope* scope, bool attach) const
{
	if (attach)
	{
		Fvector							offset[2];
		auto addon						= scope->cast<CAddon*>();
		Fmatrix trans					= (addon) ? addon->getLocalTransform() : Fidentity;
		trans.applyOffset				(scope->getSightPosition(), vZero);
		trans.getOffset					(offset);
		offset[0].z						= m_hud_offset[eIS][0].z;
		scope->setHudOffset				(offset);
	}
}

void CWeaponHud::ProcessGL(CGrenadeLauncher* gl)
{
	auto addon							= gl->cast<CAddon*>();
	Fmatrix trans						= (addon) ? addon->getLocalTransform() : Fidentity;
	trans.applyOffset					(gl->getSightOffset());
	trans.getOffset						(m_hud_offset[eGL]);
	m_hud_offset[eGL][0].z				= m_hud_offset[eIS][0].z;
}

bool CWeaponHud::IsRotatingToZoom C$()
{
	static const float eps				= .01f;

	Fvector CPC target_offset			= get_target_hud_offset();
	if (!target_offset[0].similar(m_current_hud_offset[0], eps))
		return							true;
	if (!target_offset[1].similar(m_current_hud_offset[1], eps))
		return							true;

	return								false;
}

void CWeaponHud::InitRotateTime(float cif)
{
	m_fRotateTime						= HandlingToRotationTime.Calc(cif - 1.f);
}

CWeaponHud::EHandsOffset CWeaponHud::get_target_hud_offset_idx() const
{
	if (O.IsZoomed())
	{
		switch (O.ADS())
		{
		case 0:
			return						eAim;
		case 1:
			return						(O.m_iron_sights_blockers) ? eAlt : eIS;
		case 2:
			return						eGL;
		case -1:
			return						(O.m_selected_scopes[0] && !O.m_iron_sights_blockers) ? eIS : eAlt;
		}
	}

	if (!O.ArmedMode())
	{
		if (O.HandSlot() == BOTH_HANDS_SLOT)
			return						eRelaxed;

		CEntity::SEntityState			st;
		Actor()->g_State				(st);
		if (!st.bSprint)
			return						eRelaxed;
	}

	return								eArmed;
}

Fvector CP$ CWeaponHud::get_target_hud_offset() const
{
	if (HUD().GetCurrentRayQuery().range < .7f && !smart_cast<CEntityAlive*>(Actor()->ObjectWeLookingAt()))		//--xd not accurate, doesn't take into accout weapon length
		return							m_hud_offset[eRelaxed];

	CScope* active_scope				= O.getActiveScope();
	if (active_scope)
		return							active_scope->getHudOffset();
	else
		return							m_hud_offset[get_target_hud_offset_idx()];
}

#define s_recoil_hud_angle_per_shift pSettings->r_float("weapon_manager", "recoil_hud_angle_per_shift")
#define s_recoil_hud_roll_per_shift pSettings->r_float("weapon_manager", "recoil_hud_roll_per_shift")
void CWeaponHud::UpdateHudAdditional(Fmatrix& trans)
{
	//============= Подготавливаем общие переменные =============//
	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);
	
	//============= Поворот ствола во время аима =============//
	{
		Fvector CPC target_offset = get_target_hud_offset();
		Fvector CR$ curr_offs = target_offset[0]; //pos,aim
		Fvector CR$ curr_rot = target_offset[1]; //rot,aim

		float rotate_time = m_fRotateTime;
		CScope* scope = O.getActiveScope();
		if (scope && scope->Type() == eCollimator)
			rotate_time *= .6f;
		if (!O.ADS() && O.ArmedMode())
			rotate_time *= .4f;
		float factor = Device.fTimeDelta / rotate_time;

		if (curr_offs.similar(m_current_hud_offset[0], EPS_S))
			m_current_hud_offset[0].set(curr_offs);
		else
		{
			Fvector diff = Fvector(curr_offs).sub(m_current_hud_offset[0]);
			m_current_hud_offset[0].mad(diff, factor * 2.5f);
		}

		if (curr_rot.similar(m_current_hud_offset[1], EPS_S))
			m_current_hud_offset[1].set(curr_rot);
		else
		{
			Fvector diff = Fvector(curr_rot).sub(m_current_hud_offset[1]);
			m_current_hud_offset[1].mad(diff, factor * 2.5f);
		}

		// Remove pending state before weapon has fully moved to the new position to remove some delay
		if (m_going_to_fire && curr_offs.similar(m_current_hud_offset[0], .01f) && curr_rot.similar(m_current_hud_offset[1], .01f))
		{
			O.FireStart();
			m_going_to_fire = false;
		}

		if (O.IsZoomed())
			m_fRotationFactor += factor;
		else
			m_fRotationFactor -= factor;

		clamp(m_fRotationFactor, 0.f, 1.f);
	}

	//======== Проверяем доступность инерции и стрейфа ========//
	if (!g_player_hud->inertion_allowed())
		return;

	float fYMag = Actor()->fFPCamYawMagnitude;
	float fPMag = Actor()->fFPCamPitchMagnitude;

	//============= Боковой стрейф с оружием =============//
	// Рассчитываем фактор боковой ходьбы
	hud_item_measures CR$ m = O.HudItemData()->m_measures;
	float fStrafeMaxTime = m.m_strafe_offset[2][O.IsZoomed()].y;
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
		bool bForAim = O.IsZoomed();
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

		trans.applyOffset(curr_offs, curr_rot);
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
	trans.applyOffset(cur_offs, vZero);

	if (fIsZero(O.getRecoilHudShift().magnitude()))
		trans.applyOffset(m_current_hud_offset);
	else
	{
		Fvector tmp[2] = {};
		tmp[0] = Fvector(m_current_hud_offset[0]).sub(O.m_grip_offset);
		tmp[1] = {
			O.getRecoilHudShift().x * s_recoil_hud_angle_per_shift,
			O.getRecoilHudShift().y * s_recoil_hud_angle_per_shift,
			O.getRecoilHudShift().z * s_recoil_hud_roll_per_shift
		};
		O.m_grip_offset.pivot(tmp);
		trans.applyOffset(tmp);
		trans.applyOffset(vZero, m_current_hud_offset[1]);
	}
}

extern BOOL								g_hud_adjusment_mode;
int hands_mode							= 0;
bool CWeaponHud::Action(u16 cmd, u32 flags)
{
	if (cmd == kWPN_FIRE)
	{
		if (flags&CMD_START)
		{
			if (get_target_hud_offset_idx() == eRelaxed)
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
		auto offset						= get_target_hud_offset();
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
		{
			std::ofstream				out;
			out.open					("C:\\tmp.txt");
			Fvector CP$ vp				= &offset[0];
			Fvector CP$ vr				= &offset[1];
			out							<< shared_str().printf("m_hands_offset pos [%.6f,%.6f,%.6f] m_hands_offset rot [%.6f,%.6f,%.6f]", vp->x, vp->y, vp->z, vr->x, vr->y, vr->z).c_str() << std::endl;
			return						true;
		}
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
			offset[rot][axis]			+= val;
			return						true;
		}
	}

	return false;
}

Fvector CWeaponHud::getMuzzleSightOffset() const
{
	Fvector sight_position				= vZero;
	sight_position.sub					(get_target_hud_offset()[0]);
	CScope* scope						= O.getActiveScope();
	if (scope && scope->Type() == eOptics)
		sight_position.add				(scope->getObjectiveOffset());
	return								sight_position.sub(O.m_muzzle_point);
}
