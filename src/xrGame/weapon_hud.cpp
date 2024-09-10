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

constexpr double rotation_eps = .01;

CWeaponHud::CWeaponHud(CWeaponMagazined* obj) : O(*obj)
{
	m_grip_offset						= static_cast<Dvector>(O.m_grip_point).mul(-1.);
	m_aim_z_offset						= m_grip_offset.z + static_cast<double>(pSettings->r_float(O.Section(), "aim_z_offset"));

	m_hud_offset[eRelaxed][0]			= static_cast<Dvector>(pSettings->r_fvector3(O.HudSection(), "relaxed_pos"));
	m_hud_offset[eRelaxed][1]			= static_cast<Dvector>(pSettings->r_fvector3d2r(O.HudSection(), "relaxed_rot"));
	m_grip_offset.pivot					(m_hud_offset[eRelaxed]);

	m_hud_offset[eArmed][0]				= static_cast<Dvector>(pSettings->r_fvector3(O.HudSection(), "armed_pos"));
	m_hud_offset[eArmed][1]				= static_cast<Dvector>(pSettings->r_fvector3d2r(O.HudSection(), "armed_rot"));
	m_grip_offset.pivot					(m_hud_offset[eArmed]);

	calculateAimOffsets					();
}

void CWeaponHud::calculateAimOffsets()
{
	Dvector barrel_offset				= static_cast<Dvector>(O.m_fire_point).mul(-1.);
	double aim_height					= static_cast<double>(pSettings->r_float(O.Section(), "aim_height"));
	LPCSTR hud_sect						= pSettings->r_string(O.Section(), "hud");
	
	m_hud_offset[eAlt][0]				= static_cast<Dvector>(pSettings->r_fvector3(hud_sect, "alt_aim_pos"));
	m_hud_offset[eAlt][1]				= static_cast<Dvector>(pSettings->r_fvector3(hud_sect, "alt_aim_rot"));
	barrel_offset.pivot					(m_hud_offset[eAlt]);
	m_hud_offset[eAlt][0].y				-= aim_height;
	m_hud_offset[eAlt][0].z				= m_aim_z_offset;
	
	m_hud_offset[eAim][0]				= barrel_offset;
	m_hud_offset[eAim][0].add			(static_cast<Dvector>(pSettings->r_fvector3(hud_sect, "aim_pos")));
	m_hud_offset[eAim][0].z				= m_aim_z_offset;
	m_hud_offset[eAim][1]				= dZero;

	Dvector dir							= m_hud_offset[eAim][0];
	dir.z								= 0.f;
	dir.normalize						();
	m_hud_offset[eAim][0].mad			(dir, aim_height);
}

void CWeaponHud::calculateScopeOffset(MScope* scope) const
{
	Dvector sight_position				= scope->getSightPosition();
	Dvector sight_rotation				= dZero;
	if (auto addon = scope->O.getModule<MAddon>())
	{
		addon->getLocalTransform().transform_tiny(sight_position);
		addon->getLocalTransform().getHPB(sight_rotation);
	}

	if (scope->Type() == MScope::eIS && O.m_align_front.z != dbl_max)
	{
		double dx						= O.m_align_front.x - sight_position.x;
		double dy						= O.m_align_front.y - sight_position.y;
		double dz						= O.m_align_front.z - sight_position.z;
		sight_rotation.x				+= atan(dx / dz);
		sight_rotation.y				+= atan(dy / dz);
		if (abs(sight_rotation.x) > s_iron_sights_max_angle)
			sight_rotation.x			= 0.;
		if (abs(sight_rotation.y) > s_iron_sights_max_angle)
			sight_rotation.y			= 0.;
	}
	
	Dmatrix trans						= Didentity;
	trans.setOffset						(sight_position, sight_rotation);
	trans.getOffset						(sight_position, sight_rotation);
	sight_position.z					= m_aim_z_offset;
	scope->setHudOffset					(sight_position, sight_rotation);
}

void CWeaponHud::ProcessGL(MGrenadeLauncher* gl)
{
	auto addon							= gl->O.getModule<MAddon>();
	Dmatrix trans						= (addon) ? addon->getLocalTransform() : Didentity;
	trans.applyOffset					(gl->getSightOffset());
	trans.getOffset						(m_hud_offset[eGL]);
	m_hud_offset[eGL][0].z				= m_aim_z_offset;
}

bool CWeaponHud::IsRotatingToZoom C$()
{
	Dvector CPC target_offset			= get_target_hud_offset();
	if (!target_offset[0].similar(m_current_hud_offset[0], rotation_eps))
		return							true;
	if (!target_offset[1].similar(m_current_hud_offset[1], rotation_eps))
		return							true;

	return								false;
}

float CWeaponHud::s_recoil_hud_angle_per_shift;
float CWeaponHud::s_recoil_hud_roll_per_shift;
float CWeaponHud::s_recoil_hud_rollback_per_shift;
float CWeaponHud::s_max_rotate_speed;
float CWeaponHud::s_rotate_accel_time;
float CWeaponHud::s_iron_sights_max_angle;

void CWeaponHud::loadStaticData()
{
	s_recoil_hud_angle_per_shift		= pSettings->r_float("weapon_manager", "recoil_hud_angle_per_shift");
	s_recoil_hud_roll_per_shift			= pSettings->r_float("weapon_manager", "recoil_hud_roll_per_shift");
	s_recoil_hud_rollback_per_shift		= pSettings->r_float("weapon_manager", "recoil_hud_rollback_per_shift");
	s_max_rotate_speed					= pSettings->r_float("weapon_manager", "max_rotate_speed");
	s_rotate_accel_time					= pSettings->r_float("weapon_manager", "rotate_accel_time");
	s_iron_sights_max_angle				= pSettings->r_float("weapon_manager", "iron_sights_max_angle");
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
		case -1:
			return						eAlt;
		case 2:
			return						eGL;
		}
	}

	if (!O.ArmedMode() && O.GetState() != CHudItem::eHiding && (O.GetState() != CHudItem::eShowing || O.motionPartPassed(.5f)))
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

Dvector CP$ CWeaponHud::get_target_hud_offset() const
{
	if (HUD().GetCurrentRayQuery().range < O.m_fire_point.z && !smart_cast<CEntityAlive*>(Actor()->ObjectWeLookingAt()))
		return							m_hud_offset[eRelaxed];

	if (auto active_scope = O.getActiveScope())
		return							active_scope->getHudOffset();
	else
		return							m_hud_offset[get_target_hud_offset_idx()];
}

void CWeaponHud::UpdateHudAdditional(Dmatrix& trans)
{
	//============= �������������� ����� ���������� =============//
	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);
	
	//============= ������� ������ �� ����� ���� =============//
	{
		Dvector CPC target_offset = get_target_hud_offset();
		Dvector CR$ target_rot = target_offset[1];
		Dvector target_pos = target_offset[0];
		if (O.ADS())
			target_pos.z += O.m_ads_shift;

		if (m_prev_offset != target_offset)
		{
			m_current_rotate_speed = 0.f;
			m_prev_offset = target_offset;
		}

		float factor = m_current_rotate_speed * fAvgTimeDelta / O.GetControlInertionFactor();
		if (auto scope = O.getActiveScope())
			if (target_offset == scope->getHudOffset())
				factor *= scope->getAdsSpeedFactor();

		auto inertion = [dfactor = static_cast<double>(factor)](Dvector CR$ target, Dvector& current)
		{
			if (target.similar(current, EPS_S))
				current.set(target);
			else
			{
				Dvector diff = target;
				diff.sub(current);
				current.mad(diff, dfactor);
			}
		};

		inertion(target_pos, m_current_hud_offset[0]);
		inertion(target_rot, m_current_hud_offset[1]);

		if (m_current_rotate_speed < s_max_rotate_speed)
		{
			m_current_rotate_speed += s_max_rotate_speed * fAvgTimeDelta / s_rotate_accel_time;
			if (m_current_rotate_speed > s_max_rotate_speed)
				m_current_rotate_speed = s_max_rotate_speed;
		}

		// Remove pending state before weapon has fully moved to the new position to remove some delay
		if (m_going_to_fire && target_pos.similar(m_current_hud_offset[0], rotation_eps) && target_rot.similar(m_current_hud_offset[1], rotation_eps))
		{
			O.FireStart();
			m_going_to_fire = false;
		}

		if (O.IsZoomed())
			m_fRotationFactor += factor;
		else
			m_fRotationFactor -= factor;
		clamp(m_fRotationFactor, 0.f, 1.f);

		Dvector target_d_rot = dZero;
		if (target_offset == m_hud_offset[eRelaxed])
		{
			Dvector hpb;
			trans.getHPB(hpb);
			if (hpb.y > -PI_DIV_6)
				target_d_rot.y = -hpb.y;
			else
				target_d_rot.y = PI_DIV_6;
		}
		inertion(target_d_rot, m_current_d_rot);
		trans.applyOffset(dZero, m_current_d_rot);
	}

	//======== ��������� ����������� ������� � ������� ========//
	if (!g_player_hud->inertion_allowed())
		return;

	float fYMag = Actor()->fFPCamYawMagnitude;
	float fPMag = Actor()->fFPCamPitchMagnitude;

	//============= ������� ������ � ������� =============//
	// ������������ ������ ������� ������
	hud_item_measures CR$ m = O.HudItemData()->m_measures;
	float fStrafeMaxTime = m.m_strafe_offset[2][O.IsZoomed()].y;
	// ����. ����� � ��������, �� ������� �� ���������� �� ������������ ���������
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // �������� ��������� ������� ��������

	// ��������� ������� ������ �� �������� ������
	float fCamReturnSpeedMod = 1.5f;
	// ��������� �������� ������������ �������, ����������� �� �������� ������ (������ �� �����)

	// ����������� ����������� �������� �������� ������ ��� ������ �������
	float fStrafeMinAngle = _lerp(
		m.m_strafe_offset[3][0].y,
		m.m_strafe_offset[3][1].y,
		m_fRotationFactor);

	// ����������� ����������� ������ �� �������� ������
	float fCamLimitBlend = _lerp(
		m.m_strafe_offset[3][0].x,
		m.m_strafe_offset[3][1].x,
		m_fRotationFactor);

	// ������� ������ �� �������� ������
	if (abs(fYMag) > (O.m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{
		//--> ������ �������� �� ��� Y
		O.m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
		clamp(O.m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{
		bool bForAim = O.IsZoomed();
		//--> ������ �� �������������� - ������� ������
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

	// ��������� ������� ������ �� ������ ����
	float fChangeDirSpeedMod = 3;
	// ��������� ������ ������ ����������� ����������� �������, ���� ��� � ������ ������� �� ��������
	u32 iMovingState = Actor()->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{
		// �������� �����
		float fVal = (O.m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		O.m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{
		// �������� ������
		float fVal = (O.m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		O.m_fLR_MovingFactor += fVal;
	}
	else
	{
		// ��������� � ����� ������ ����������� - ������ ������� ������
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
	clamp(O.m_fLR_MovingFactor, -1.0f, 1.0f); // ������ ������� ������ �� ������ ��������� ��� ������

	// ��������� � ������������� �������� ������ �������
	float fLR_Factor = O.m_fLR_MovingFactor; 
	fLR_Factor += O.m_fLR_CameraFactor;

	clamp(fLR_Factor, -1.0f, 1.0f); // ������ ������� ������ �� ������ ��������� ��� ������

	// ���������� ������ ������ ��� ����������� ������ � ����
	for (int _idx = 0; _idx <= 1; _idx++) //<-- ��� �������� ��������
	{
		bool bEnabled = (m.m_strafe_offset[2][_idx].x != 0.0f);
		if (!bEnabled)
			continue;

		Fvector curr_offs, curr_rot;

		// �������� ������� ���� � �������
		curr_offs = m.m_strafe_offset[0][_idx]; // pos
		curr_offs.mul(fLR_Factor); // �������� �� ������ �������

		// ������� ���� � �������
		curr_rot = m.m_strafe_offset[1][_idx]; // rot
		curr_rot.mul(-PI / 180.f); // ����������� ���� � �������
		curr_rot.mul(fLR_Factor); // �������� �� ������ �������

		// ������ ������� ����� ������ \ ��������
		if (_idx == 0)
		{
			// �� �����
			curr_offs.mul(1.f - m_fRotationFactor);
			curr_rot.mul(1.f - m_fRotationFactor);
		}
		else
		{
			// �� ����� ����
			curr_offs.mul(m_fRotationFactor);
			curr_rot.mul(m_fRotationFactor);
		}

		trans.applyOffset(static_cast<Dvector>(curr_offs), static_cast<Dvector>(curr_rot));
	}

	//============= ������� ������ =============//
	// ��������� �������
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

	// ����������� ������� �� ��������� ������
	bool bIsInertionPresent = !fIsZero(O.m_fLR_InertiaFactor) || !fIsZero(O.m_fUD_InertiaFactor);
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && O.m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && O.m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> �������� ������� ��� �������� � ��������������� �������
		}

		O.m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // ����������� (�.�. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && O.m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && O.m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> �������� ������� ��� �������� � ��������������� �������
		}

		O.m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // ��������� (�.�. > |1.0|)
	}

	clamp(O.m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(O.m_fUD_InertiaFactor, -1.0f, 1.0f);

	// ������� ��������� ������� (��������, �� ��� �������� ������� �� ������� ������� �� ������� 0.0f)
	O.m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	O.m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// ����������� �������� ��������� ������� ��� ����� (�����������)
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

	// ����������� �������� ��������� ������� ��� ����� (���������)
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

	// ��������� ������� � ����
	float fLR_lim = (O.m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (O.m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	Fvector cur_offs = { fLR_lim * -1.f * O.m_fLR_InertiaFactor, fUD_lim * O.m_fUD_InertiaFactor, 0.0f };
	trans.translate_mul(static_cast<Dvector>(cur_offs));

	if (fIsZero(O.getRecoilHudShift().magnitude()))
		trans.applyOffset(m_current_hud_offset);
	else
	{
		Dvector tmp[2] = {
			Dvector(m_current_hud_offset[0]).sub(static_cast<Dvector>(m_grip_offset)),
			{
				O.getRecoilHudShift().x * s_recoil_hud_angle_per_shift,
				O.getRecoilHudShift().y * s_recoil_hud_angle_per_shift,
				O.getRecoilHudShift().z * s_recoil_hud_roll_per_shift
			}
		};
		tmp[0].z -= O.getRecoilHudShift().w * s_recoil_hud_rollback_per_shift;
		m_grip_offset.pivot(tmp);
		trans.applyOffset(tmp);
		trans.applyOffset(dZero, m_current_hud_offset[1]);
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
		case kLEFT:
		case kRIGHT:
			axis						= 0;
			break;
		case kFWD:
		case kBACK:
		case kUP:
		case kDOWN:
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
			auto CP$ vp					= &offset[0];
			auto CP$ vr					= &offset[1];
			out							<< shared_str().printf("m_hands_offset pos [%.6f,%.6f,%.6f] m_hands_offset rot [%.6f,%.6f,%.6f]", vp->x, vp->y, vp->z, vr->x, vr->y, vr->z).c_str() << std::endl;
			return						true;
		}
		}

		if (axis != -1)
		{
			float val					= 0.0001f;
			if (cmd == kL_STRAFE || cmd == kRIGHT || cmd == kBACK || cmd == kDOWN || cmd == kL_LOOKOUT)
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

Fvector CWeaponHud::getTransference(float distance) const
{
	Dmatrix								cur_trans;
	cur_trans.setOffset					(get_target_hud_offset());
	if (auto scope = O.getActiveScope())
		if (scope->Type() == MScope::eOptics)
			cur_trans.translate_sub		(scope->getObjectiveOffset());
	Fmatrix f_cur_trans					= static_cast<Fmatrix>(cur_trans);

	f_cur_trans.translate_mul			(O.m_fire_point);
	Fmatrix								i_cur_trans;
	i_cur_trans.invert					(f_cur_trans);

	Fvector shoot_dir					= vForward;
	i_cur_trans.transform_dir			(shoot_dir);

	return								i_cur_trans.c.mad(shoot_dir, distance);
}
