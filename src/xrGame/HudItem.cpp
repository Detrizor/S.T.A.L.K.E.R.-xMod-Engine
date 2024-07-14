#include "stdafx.h"
#include "HudItem.h"
#include "physic_item.h"
#include "actor.h"
#include "actoreffector.h"
#include "Missile.h"
#include "xrmessages.h"
#include "level.h"
#include "inventory.h"
#include "../xrEngine/CameraBase.h"
#include "player_hud.h"
#include "../xrEngine/SkeletonMotions.h"

#include "../build_config_defines.h"
#include "ui_base.h"

#include "script_callback_ex.h"
#include "script_game_object.h"

CHudItem::CHudItem()
{
	RenderHud(TRUE);
	EnableHudInertion(TRUE);
	AllowHudInertion(TRUE);
	m_bStopAtEndAnimIsRunning = false;
	m_current_motion_def = NULL;
	m_started_rnd_anim_idx = u8(-1);

	m_fLR_CameraFactor = 0.f;
	m_fLR_MovingFactor = 0.f;
	m_fLR_InertiaFactor = 0.f;
	m_fUD_InertiaFactor = 0.f;
}

DLL_Pure *CHudItem::_construct()
{
	m_object = smart_cast<CPhysicItem*>(this);
	VERIFY(m_object);

	m_item = smart_cast<CInventoryItem*>(this);
	VERIFY(m_item);

	return				(m_object);
}

void CHudItem::Load(LPCSTR section)
{
	hud_sect							= pSettings->r_string(section, "hud");
	m_animation_slot					= pSettings->r_u32(section, "animation_slot");
	m_sounds.LoadSound					(section, "snd_bore", "sndBore", true);
	m_using_blend_idle_anims			= !!READ_IF_EXISTS(pSettings, r_bool, hud_sect, "using_blend_idle_anims", TRUE);
	
	if (pSettings->line_exist(hud_sect, "show_anm"))
	{
		m_show_anm.name					= pSettings->r_string(hud_sect, "show_anm");
		m_show_anm.speed				= pSettings->r_float(hud_sect, "show_anm_speed");
		m_show_anm.power				= pSettings->r_float(hud_sect, "show_anm_power");
	}
	if (pSettings->line_exist(hud_sect, "hide_anm"))
	{
		m_hide_anm.name				= pSettings->r_string(hud_sect, "hide_anm");
		m_hide_anm.speed				= pSettings->r_float(hud_sect, "hide_anm_speed");
		m_hide_anm.power				= pSettings->r_float(hud_sect, "hide_anm_power");
	}
}

void CHudItem::PlaySound(LPCSTR alias, const Fvector& position)
{
	m_sounds.PlaySound(alias, position, object().H_Root(), !!GetHUDmode());
}

//Alundaio: Play at index
void CHudItem::PlaySound(LPCSTR alias, const Fvector& position, u8 index)
{
	m_sounds.PlaySound(alias, position, object().H_Root(), !!GetHUDmode(), false, index);
}
//-Alundaio

void CHudItem::renderable_Render()
{
	UpdateXForm();
	BOOL _hud_render = ::Render->get_HUD() && GetHUDmode();

	if (_hud_render  && !IsHidden())
	{
	}
	else
	{
		if (!object().H_Parent() || (!_hud_render && !IsHidden()))
		{
			on_renderable_Render();
			debug_draw_firedeps();
		}
		else
			if (object().H_Parent())
			{
				CInventoryOwner	*owner = smart_cast<CInventoryOwner*>(object().H_Parent());
				VERIFY(owner);
				CInventoryItem	*self = smart_cast<CInventoryItem*>(this);
				if (owner->attached(self))
					on_renderable_Render();
			}
	}
}

void CHudItem::SwitchState(u32 S)
{
	OnStateSwitch(S, GetState());
}

void CHudItem::OnStateSwitch(u32 S, u32 oldState)
{
	SetState(S);

	switch (S)
	{
	case eShowing:
		if (HudAnimationExist("anm_show"))
			PlayHUDMotion("anm_show", FALSE, S);
		else
		{
			auto anm = playBlendAnm(m_show_anm.name, m_show_anm.speed, m_show_anm.power, false);
			anm->blend_amount = 1.f;
		}
		break;
	case eHiding:
		if (oldState != eHiding)
		{
			SetPending(TRUE);
			if (HudAnimationExist("anm_hide"))
				PlayHUDMotion("anm_hide", FALSE, S);
			else
				playBlendAnm(m_hide_anm.name, m_hide_anm.speed, m_hide_anm.power, false);
		}
		break;
	case eIdle:
		PlayAnimIdle();
		break;
	case eBore:
		SetPending(FALSE);

		PlayAnimBore();
		if (HudItemData())
		{
			Fvector P = static_cast<Fvector>(HudItemData()->m_transform.c);
			m_sounds.PlaySound("sndBore", P, object().H_Root(), !!GetHUDmode(), false, m_started_rnd_anim_idx);
		}

		break;
	}
}

void CHudItem::OnAnimationEnd(u32 state)
{
	CActor* A = smart_cast<CActor*>(object().H_Parent());
	if (A)
		A->callback(GameObject::eActorHudAnimationEnd)(smart_cast<CGameObject*>(this)->lua_game_object(),this->hud_sect.c_str(), this->m_current_motion.c_str(), state, this->animation_slot());

	switch (state)
	{
	case eHiding:
		SwitchState(eHidden);
		break;
	case eIdle:
		break;
	default:
		SwitchState(eIdle);
	}
}

void CHudItem::PlayAnimBore()
{
	PlayHUDMotion("anm_bore", TRUE, GetState());
}

bool CHudItem::ActivateItem()
{
	OnActiveItem();
	return			true;
}

void CHudItem::DeactivateItem()
{
	OnHiddenItem();
}
void CHudItem::OnMoveToRuck(const SInvItemPlace& prev)
{
	SwitchState(eHidden);
}

void CHudItem::SendDeactivateItem()
{
	SendHiddenItem();
}
void CHudItem::SendHiddenItem()
{
	SwitchState(eHiding);
}

void CHudItem::UpdateHudAdditional(Dmatrix& trans)
{
	CActor* pActor = smart_cast<CActor*>(object().H_Parent());
	if (!pActor)
		return;

	attachable_hud_item* hi = HudItemData();
	R_ASSERT(hi);

	if (!g_player_hud->inertion_allowed())
		return;

	static float fAvgTimeDelta = Device.fTimeDelta;
	fAvgTimeDelta = _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

	float fYMag = pActor->fFPCamYawMagnitude;
	float fPMag = pActor->fFPCamPitchMagnitude;

	float fStrafeMaxTime = hi->m_measures.m_strafe_offset[2][0].y;
	// Макс. время в секундах, за которое мы наклонимся из центрального положения
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

	// Добавляем боковой наклон от движения камеры
	float fCamReturnSpeedMod = 1.5f;
	// Восколько ускоряем нормализацию наклона, полученного от движения камеры (только от бедра)

	// Высчитываем минимальную скорость поворота камеры для начала инерции
	float fStrafeMinAngle = hi->m_measures.m_strafe_offset[3][0].y;

	// Высчитываем мксимальный наклон от поворота камеры
	float fCamLimitBlend = hi->m_measures.m_strafe_offset[3][0].x;

	// Считаем стрейф от поворота камеры
	if (abs(fYMag) > (m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{
		//--> Камера крутится по оси Y
		m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
		clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{
		//--> Камера не поворачивается - убираем наклон
		if (m_fLR_CameraFactor < 0.0f)
		{
			m_fLR_CameraFactor += fStepPerUpd * fCamReturnSpeedMod;
			clamp(m_fLR_CameraFactor, -fCamLimitBlend, 0.0f);
		}
		else
		{
			m_fLR_CameraFactor -= fStepPerUpd * fCamReturnSpeedMod;
			clamp(m_fLR_CameraFactor, 0.0f, fCamLimitBlend);
		}
	}

	// Добавляем боковой наклон от ходьбы вбок
	float fChangeDirSpeedMod = 3;
	// Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего
	u32 iMovingState = pActor->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{
		// Движемся влево
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{
		// Движемся вправо
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else
	{
		// Двигаемся в любом другом направлении - плавно убираем наклон
		if (m_fLR_MovingFactor < 0.0f)
		{
			m_fLR_MovingFactor += fStepPerUpd;
			clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_MovingFactor -= fStepPerUpd;
			clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}
	clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	// Вычисляем и нормализируем итоговый фактор наклона
	float fLR_Factor = m_fLR_MovingFactor;

	// No cam strafe inertia while in freelook mode
	//--xd на будущее if (pActor->cam_freelook == eflDisabled)
		fLR_Factor += m_fLR_CameraFactor;

	clamp(fLR_Factor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

	Dvector curr_offs, curr_rot;

	if (hi->m_measures.m_strafe_offset[2][0].x != 0.f)
	{
		// Смещение позиции худа в стрейфе
		curr_offs = static_cast<Dvector>(hi->m_measures.m_strafe_offset[0][0]); // pos
		curr_offs.mul(static_cast<double>(fLR_Factor)); // Умножаем на фактор стрейфа

		// Поворот худа в стрейфе
		curr_rot = static_cast<Dvector>(hi->m_measures.m_strafe_offset[1][0]); // rot
		curr_rot.mul(-deg2rad(1.)); // Преобразуем углы в радианы
		curr_rot.mul(static_cast<double>(fLR_Factor)); // Умножаем на фактор стрейфа

		trans.applyOffset(curr_offs, curr_rot);
	}

	//============= Инерция оружия =============//
	// Параметры инерции
	float fInertiaSpeedMod = hi->m_measures.m_inertion_params.m_tendto_speed;

	float fInertiaReturnSpeedMod = hi->m_measures.m_inertion_params.m_tendto_ret_speed;

	float fInertiaMinAngle = hi->m_measures.m_inertion_params.m_min_angle;

	Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
	vIOffsets.x = hi->m_measures.m_inertion_params.m_offset_LRUD.x;
	vIOffsets.y = hi->m_measures.m_inertion_params.m_offset_LRUD.y;
	vIOffsets.z = hi->m_measures.m_inertion_params.m_offset_LRUD.z;
	vIOffsets.w = hi->m_measures.m_inertion_params.m_offset_LRUD.w;

	// Высчитываем инерцию из поворотов камеры
	bool bIsInertionPresent = m_fLR_InertiaFactor != 0.0f || m_fUD_InertiaFactor != 0.0f;
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Горизонталь (м.б. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
		}

		m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Вертикаль (м.б. > |1.0|)
	}

	clamp(m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(m_fUD_InertiaFactor, -1.0f, 1.0f);

	// Плавное затухание инерции (основное, но без линейной никогда не опустит инерцию до полного 0.0f)
	m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// Минимальное линейное затухание инерции при покое (горизонталь)
	if (fYMag == 0.0f)
	{
		float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (m_fLR_InertiaFactor < 0.0f)
		{
			m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fLR_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Минимальное линейное затухание инерции при покое (вертикаль)
	if (fPMag == 0.0f)
	{
		float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
		if (m_fUD_InertiaFactor < 0.0f)
		{
			m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_InertiaFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
			clamp(m_fUD_InertiaFactor, 0.0f, 1.0f);
		}
	}

	// Применяем инерцию к худу
	float fLR_lim = (m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	curr_offs = { fLR_lim * -1.f * m_fLR_InertiaFactor, fUD_lim * m_fUD_InertiaFactor, 0.0f };

	trans.translate_mul(curr_offs);
}

void CHudItem::UpdateCL()
{
	if (m_current_motion_def)
	{
		if (m_bStopAtEndAnimIsRunning)
		{
			const xr_vector<motion_marks>&	marks = m_current_motion_def->marks;
			if (!marks.empty())
			{
				float motion_prev_time = ((float) m_dwMotionCurrTm - (float) m_dwMotionStartTm) / 1000.0f;
				float motion_curr_time = ((float) Device.dwTimeGlobal - (float) m_dwMotionStartTm) / 1000.0f;

				xr_vector<motion_marks>::const_iterator it = marks.begin();
				xr_vector<motion_marks>::const_iterator it_e = marks.end();
				for (; it != it_e; ++it)
				{
					const motion_marks&	M = (*it);
					if (M.is_empty())
						continue;

					const motion_marks::interval* Iprev = M.pick_mark(motion_prev_time);
					const motion_marks::interval* Icurr = M.pick_mark(motion_curr_time);
					if (Iprev == NULL && Icurr != NULL /* || M.is_mark_between(motion_prev_time, motion_curr_time)*/)
					{
						OnMotionMark(m_startedMotionState, M);
					}
				}
			}

			m_dwMotionCurrTm = Device.dwTimeGlobal;
			if (m_dwMotionCurrTm > m_dwMotionEndTm)
			{
				m_current_motion_def = NULL;
				m_dwMotionStartTm = 0;
				m_dwMotionEndTm = 0;
				m_dwMotionCurrTm = 0;
				m_bStopAtEndAnimIsRunning = false;
				OnAnimationEnd(m_startedMotionState);
			}
		}
	}
}

void CHudItem::OnH_A_Chield()
{}

void CHudItem::OnH_B_Chield()
{
	StopCurrentAnimWithoutCallback();
}

void CHudItem::OnH_B_Independent(bool just_before_destroy)
{
	m_sounds.StopAllSounds();
	UpdateXForm();

	// next code was commented
	/*
	if(HudItemData() && !just_before_destroy)
	{
	object().XFORM().set( HudItemData()->m_transform );
	}

	if (HudItemData())
	{
	g_player_hud->detach_item(this);
	Msg("---Detaching hud item [%s][%d]", this->HudSection().c_str(), this->object().ID());
	}*/
	//SetHudItemData			(NULL);
}

void CHudItem::OnH_A_Independent()
{
	if (HudItemData())
		g_player_hud->detach_item(this);
	StopCurrentAnimWithoutCallback();
}

void CHudItem::on_b_hud_detach()
{
	m_sounds.StopAllSounds();
}

void CHudItem::on_a_hud_attach()
{
	if (m_current_motion_def)
	{
		PlayHUDMotion_noCB(m_current_motion, FALSE);
#ifdef DEBUG
		//		Msg("continue playing [%s][%d]",m_current_motion.c_str(), Device.dwFrame);
#endif // #ifdef DEBUG
	}
	else
	{
#ifdef DEBUG
		//		Msg("no active motion");
#endif // #ifdef DEBUG
	}
}

u32 CHudItem::PlayHUDMotion(shared_str name, BOOL bMixIn, u32 state)
{
	shared_str							tmp;
	if (get_anm_prefix())
	{
		tmp.printf						("anm_%s_%s", get_anm_prefix(), (*name + 4));
		if (HudAnimationExist(*tmp))
			name						= tmp;
	}

	if (HudItemData() && !HudAnimationExist(*name))
	{
		Msg("!Missing hud animation %s", *name);
		return 0;
	}

	u32 anim_time = PlayHUDMotion_noCB(name, bMixIn);
	if (anim_time > 0)
	{
		m_bStopAtEndAnimIsRunning = true;
		m_dwMotionStartTm = Device.dwTimeGlobal;
		m_dwMotionCurrTm = m_dwMotionStartTm;
		m_dwMotionEndTm = m_dwMotionStartTm + anim_time;
		m_startedMotionState = state;
	}
	else
		m_bStopAtEndAnimIsRunning = false;

	return anim_time;
}

u32 CHudItem::PlayHUDMotion_noCB(const shared_str& motion_name, BOOL bMixIn)
{
	m_current_motion = motion_name;

	if (bDebug && item().m_pInventory)
	{
		Msg("-[%s] as[%d] [%d]anim_play [%s][%d]",
			HudItemData() ? "HUD" : "Simulating",
			item().m_pInventory->GetActiveSlot(),
			item().object_id(),
			motion_name.c_str(),
			Device.dwFrame);
	}
	if (HudItemData())
	{
		return HudItemData()->anim_play(motion_name, bMixIn, m_current_motion_def, m_started_rnd_anim_idx);
	}
	else
	{
		m_started_rnd_anim_idx = 0;
		return g_player_hud->motion_length(motion_name, HudSection(), m_object->cNameSect(), m_current_motion_def);
	}
}

void CHudItem::StopCurrentAnimWithoutCallback()
{
	m_dwMotionStartTm = 0;
	m_dwMotionEndTm = 0;
	m_dwMotionCurrTm = 0;
	m_bStopAtEndAnimIsRunning = false;
	m_current_motion_def = NULL;
}

BOOL CHudItem::GetHUDmode()
{
	if (object().H_Parent())
	{
		CActor* A = smart_cast<CActor*>(object().H_Parent());
		return (A && A->HUDview() && HudItemData());
	}
	else
		return FALSE;
}

void CHudItem::PlayAnimIdle()
{
	if (m_using_blend_idle_anims || !TryPlayAnimIdle())
		PlayHUDMotion("anm_idle", TRUE, GetState());
}

bool CHudItem::TryPlayAnimIdle()
{
	auto actor = smart_cast<CActor*>(object().H_Parent());
	auto wpn = object().Cast<CWeapon*>();
	bool ads = wpn && wpn->ADS();

	if (actor && actor->AnyMove())
	{
		if (ads)
		{
			PlayHUDMotion("anm_idle_aim_moving", TRUE, GetState());
			return true;
		}

		CEntity::SEntityState st;
		actor->g_State(st);
		if (st.bSprint)
		{
			if (wpn && !wpn->ArmedMode() && wpn->HandSlot() == BOTH_HANDS_SLOT)
				PlayHUDMotion("anm_idle_moving", TRUE, GetState());
			else
				PlayHUDMotion("anm_idle_sprint", TRUE, GetState());
			return true;
		}
		else if (st.bCrouch)
		{
			PlayHUDMotion((HudAnimationExist("anm_idle_moving_crouch")) ? "anm_idle_moving_crouch" : "anm_idle_moving", TRUE, GetState());
			return true;
		}
		else
		{
			PlayHUDMotion("anm_idle_moving", TRUE, GetState());
			return true;
		}
	}

	if (ads)
	{
		PlayHUDMotion("anm_idle_aim", TRUE, GetState());
		return true;
	}

	return false;
}

//AVO: check if animation exists
bool CHudItem::HudAnimationExist(LPCSTR anim_name)
{
	if (HudItemData()) // First person
	{
		if (HudItemData()->m_hand_motions.find_motion(anim_name))
			return true;
	}
	else // Third person
	{
		if (g_player_hud->motion_length(anim_name, HudSection(), m_object->cNameSect(), m_current_motion_def) > 100)
			return true;
	}
#ifdef DEBUG
	Msg("~ [WARNING] ------ Animation [%s] does not exist in [%s]", anim_name, HudSection().c_str());
#endif
	return false;
}
//-AVO

void CHudItem::onMovementChanged()
{
	if (GetState() == eIdle && !m_bStopAtEndAnimIsRunning)
	{
		ResetSubStateTime();
		if (!m_using_blend_idle_anims)
			PlayAnimIdle();
	}
}

attachable_hud_item* CHudItem::HudItemData() const
{
	attachable_hud_item* hi = NULL;
	if (!g_player_hud)
		return				hi;

	hi = g_player_hud->attached_item(0);
	if (hi && hi->m_parent_hud_item == this)
		return hi;

	hi = g_player_hud->attached_item(1);
	if (hi && hi->m_parent_hud_item == this)
		return hi;

	return NULL;
}

script_layer* CHudItem::playBlendAnm(shared_str CR$ name, float speed, float power, bool stop_old) const
{
	if (stop_old)
		g_player_hud->StopBlendAnm		(name, true);
	u8 part								= (object().cast_weapon()->IsZoomed()) ? 2 : ((g_player_hud->attached_item(1)) ? 0 : 2);
	return								g_player_hud->playBlendAnm(name, part, speed, power, false, false, GetState());
}

void CHudItem::UpdateSlotsTransform()
{
	m_object->Aboba						(eUpdateSlotsTransform);
}

void CHudItem::UpdateHudBonesVisibility()
{
	m_object->Aboba						(eUpdateHudBonesVisibility);
}

void CHudItem::render_hud_mode()
{
	m_object->Aboba						(eRenderHudMode);
}
