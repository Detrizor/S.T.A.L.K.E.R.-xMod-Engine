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

void SScriptAnm::load(shared_str CR$ section, LPCSTR name)
{
	if (!pSettings->line_exist(section, name))
		return;
	
	LPCSTR S							= pSettings->r_string(section, name);
	if (S && S[0])
	{
		string128						item;
		_GetItem						(S, 0, item);
		anm								= item;
		
		int count						= _GetItemCount(S);
		if (count >= 2)
		{
			_GetItem					(S, 1, item);
			speed						= atof(item);
		}
		if (count >= 3)
		{
			_GetItem					(S, 2, item);
			power						= atof(item);
		}
	}
}

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
	
	m_show_anm.load						(hud_sect, "show_anm");
	m_hide_anm.load						(hud_sect, "hide_anm");
		
	m_draw_anm_primary.load				(hud_sect, "draw_anm_primary");
	m_draw_anm_secondary.load			(hud_sect, "draw_anm_secondary");
	m_holster_anm_primary.load			(hud_sect, "holster_anm_primary");
	m_holster_anm_secondary.load		(hud_sect, "holster_anm_secondary");
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
		SetPending(TRUE);
		if (anm_slot && HudAnimationExist("anm_show"))
			PlayHUDMotion("anm_show", FALSE, S);
		else if (anm_slot == PRIMARY_SLOT)
			playBlendAnm(m_draw_anm_primary, S, true);
		else if (anm_slot == SECONDARY_SLOT)
			playBlendAnm(m_draw_anm_secondary, S, true);
		else
			playBlendAnm(m_show_anm, S, true);
		break;
	case eHiding:
		if (oldState != eHiding)
		{
			SetPending(TRUE);
			if (anm_slot && HudAnimationExist("anm_hide"))
				PlayHUDMotion("anm_hide", FALSE, S);
			else if (anm_slot == PRIMARY_SLOT)
				playBlendAnm(m_holster_anm_primary, S);
			else if (anm_slot == SECONDARY_SLOT)
				playBlendAnm(m_holster_anm_secondary, S);
			else
				playBlendAnm(m_hide_anm, S);
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
	case eHidden:
		g_player_hud->detach_item(this);
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

bool CHudItem::activateItem(u16 prev_slot)
{
	if (prev_slot != u16_max)
		anm_slot = prev_slot;
	OnActiveItem();
	return			true;
}

void CHudItem::deactivateItem(u16 slot)
{
	if (slot != u16_max)
		anm_slot = slot;
	OnHiddenItem();
}

void CHudItem::hideItem()
{
	anm_slot = 0;
	OnHiddenItem();
}

void CHudItem::restoreItem()
{
	anm_slot = 0;
	OnActiveItem();
}

void CHudItem::OnActiveItem()
{
	if (object().H_Parent()->scast<CActor*>())
		g_player_hud->attach_item(this);

	SwitchState((Device.dwPrecacheFrame) ? eIdle : eShowing);
}

void CHudItem::OnMoveToRuck(const SInvItemPlace& prev)
{
	if (GetState() != eHidden)
		SwitchState(eHidden);
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
	// ����. ����� � ��������, �� ������� �� ���������� �� ������������ ���������
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // �������� ��������� ������� ��������

	// ��������� ������� ������ �� �������� ������
	float fCamReturnSpeedMod = 1.5f;
	// ��������� �������� ������������ �������, ����������� �� �������� ������ (������ �� �����)

	// ����������� ����������� �������� �������� ������ ��� ������ �������
	float fStrafeMinAngle = hi->m_measures.m_strafe_offset[3][0].y;

	// ����������� ����������� ������ �� �������� ������
	float fCamLimitBlend = hi->m_measures.m_strafe_offset[3][0].x;

	// ������� ������ �� �������� ������
	if (abs(fYMag) > (m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
	{
		//--> ������ �������� �� ��� Y
		m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
		clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
	}
	else
	{
		//--> ������ �� �������������� - ������� ������
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

	// ��������� ������� ������ �� ������ ����
	float fChangeDirSpeedMod = 3;
	// ��������� ������ ������ ����������� ����������� �������, ���� ��� � ������ ������� �� ��������
	u32 iMovingState = pActor->MovingState();
	if ((iMovingState & mcLStrafe) != 0)
	{
		// �������� �����
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{
		// �������� ������
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else
	{
		// ��������� � ����� ������ ����������� - ������ ������� ������
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
	clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // ������ ������� ������ �� ������ ��������� ��� ������

	// ��������� � ������������� �������� ������ �������
	float fLR_Factor = m_fLR_MovingFactor;

	// No cam strafe inertia while in freelook mode
	//--xd �� ������� if (pActor->cam_freelook == eflDisabled)
		fLR_Factor += m_fLR_CameraFactor;

	clamp(fLR_Factor, -1.0f, 1.0f); // ������ ������� ������ �� ������ ��������� ��� ������

	Dvector curr_offs, curr_rot;

	if (hi->m_measures.m_strafe_offset[2][0].x != 0.f)
	{
		// �������� ������� ���� � �������
		curr_offs = static_cast<Dvector>(hi->m_measures.m_strafe_offset[0][0]); // pos
		curr_offs.mul(static_cast<double>(fLR_Factor)); // �������� �� ������ �������

		// ������� ���� � �������
		curr_rot = static_cast<Dvector>(hi->m_measures.m_strafe_offset[1][0]); // rot
		curr_rot.mul(-deg2rad(1.)); // ����������� ���� � �������
		curr_rot.mul(static_cast<double>(fLR_Factor)); // �������� �� ������ �������

		trans.applyOffset(curr_offs, curr_rot);
	}

	//============= ������� ������ =============//
	// ��������� �������
	float fInertiaSpeedMod = hi->m_measures.m_inertion_params.m_tendto_speed;

	float fInertiaReturnSpeedMod = hi->m_measures.m_inertion_params.m_tendto_ret_speed;

	float fInertiaMinAngle = hi->m_measures.m_inertion_params.m_min_angle;

	Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
	vIOffsets.x = hi->m_measures.m_inertion_params.m_offset_LRUD.x;
	vIOffsets.y = hi->m_measures.m_inertion_params.m_offset_LRUD.y;
	vIOffsets.z = hi->m_measures.m_inertion_params.m_offset_LRUD.z;
	vIOffsets.w = hi->m_measures.m_inertion_params.m_offset_LRUD.w;

	// ����������� ������� �� ��������� ������
	bool bIsInertionPresent = m_fLR_InertiaFactor != 0.0f || m_fUD_InertiaFactor != 0.0f;
	if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fYMag > 0.0f && m_fLR_InertiaFactor > 0.0f ||
			fYMag < 0.0f && m_fLR_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> �������� ������� ��� �������� � ��������������� �������
		}

		m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // ����������� (�.�. > |1.0|)
	}

	if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
	{
		float fSpeed = fInertiaSpeedMod;
		if (fPMag > 0.0f && m_fUD_InertiaFactor > 0.0f ||
			fPMag < 0.0f && m_fUD_InertiaFactor < 0.0f)
		{
			fSpeed *= 2.f; //--> �������� ������� ��� �������� � ��������������� �������
		}

		m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // ��������� (�.�. > |1.0|)
	}

	clamp(m_fLR_InertiaFactor, -1.0f, 1.0f);
	clamp(m_fUD_InertiaFactor, -1.0f, 1.0f);

	// ������� ��������� ������� (��������, �� ��� �������� ������� �� ������� ������� �� ������� 0.0f)
	m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
	m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

	// ����������� �������� ��������� ������� ��� ����� (�����������)
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

	// ����������� �������� ��������� ������� ��� ����� (���������)
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

	// ��������� ������� � ����
	float fLR_lim = (m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
	float fUD_lim = (m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

	curr_offs = { fLR_lim * -1.f * m_fLR_InertiaFactor, fUD_lim * m_fUD_InertiaFactor, 0.0f };

	trans.translate_mul(curr_offs);
}

void CHudItem::UpdateCL()
{
	if (m_bStopAtEndAnimIsRunning)
	{
		if (m_current_motion_def)
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
		}

		m_dwMotionCurrTm = Device.dwTimeGlobal;
		if (m_dwMotionCurrTm > m_dwMotionEndTm)
		{
			if (m_current_anm.size())
				g_player_hud->StopBlendAnm(m_current_anm, true);
			m_current_motion_def = nullptr;
			m_dwMotionStartTm = 0;
			m_dwMotionEndTm = 0;
			m_dwMotionCurrTm = 0;
			m_bStopAtEndAnimIsRunning = false;
			OnAnimationEnd(m_startedMotionState);
		}
	}
}

void CHudItem::activate_physic_shell()
{
	if (!smart_cast<CEntityAlive*>(object().H_Parent()))
	{
		on_activate_physic_shell();
		return;
	}
	
	if (auto hi = HudItemData())
		object().XFORM().mul(static_cast<Fmatrix>(HudItemData()->m_transform), hi->m_model->LL_GetTransform_R(0));
	else
		UpdateXForm();

	object().CPhysicsShellHolder::activate_physic_shell();
}

void CHudItem::OnH_A_Chield()
{}

void CHudItem::OnH_B_Chield()
{
	StopCurrentAnimWithoutCallback();
}

void CHudItem::OnH_B_Independent(bool just_before_destroy)
{
	if (GetState() != eHidden)
	{
		OnHiddenItem();
		SwitchState(eHidden);
	}
	SetPending(FALSE);
	m_sounds.StopAllSounds();
}

void CHudItem::OnH_A_Independent()
{
	StopCurrentAnimWithoutCallback();
}

void CHudItem::on_b_hud_detach()
{
	m_sounds.StopAllSounds();
}

void CHudItem::on_a_hud_attach()
{
}

u32 CHudItem::PlayHUDMotion(shared_str name, BOOL bMixIn, u32 state, bool ignore_anm_type)
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

	u32 anim_time = PlayHUDMotion_noCB(name, bMixIn, ignore_anm_type);
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

u32 CHudItem::PlayHUDMotion_noCB(const shared_str& motion_name, BOOL bMixIn, bool ignore_anm_type)
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
		return HudItemData()->anim_play(motion_name, bMixIn, m_current_motion_def, m_started_rnd_anim_idx, ignore_anm_type);

	m_started_rnd_anim_idx = 0;
	return g_player_hud->motion_length(motion_name, HudSection(), m_object->cNameSect(), m_current_motion_def);
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
	SetPending							(FALSE);

	if (HudAnimationExist("anm_idle_sprint"))
	{
		auto actor						= smart_cast<CActor*>(object().H_Parent());
		if (actor && actor->AnyMove())
		{
			CEntity::SEntityState		 st;
			actor->g_State				(st);
			if (st.bSprint)
			{
				auto wpn				= object().scast<CWeapon*>();
				if (wpn && !wpn->ArmedMode())
				{
					PlayHUDMotion		("anm_idle_sprint", TRUE, GetState());
					return;
				}
			}
		}
	}

	PlayHUDMotion("anm_idle", TRUE, GetState());
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
	if (!m_bStopAtEndAnimIsRunning)
		PlayHUDMotion("anm_idle", TRUE, GetState());
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

void CHudItem::playBlendAnm(SScriptAnm CR$ anm, u32 state, bool full_blend, float power_k)
{
	float								 anim_time;
	if (HudItemData())
	{
		auto weapon						= object().cast_weapon();
		u8 part							= (weapon && weapon->IsZoomed()) ? 2 : ((g_player_hud->attached_item(1)) ? 0 : 2);
		auto layer						= g_player_hud->playBlendAnm(anm.anm, part, anm.speed, anm.power * power_k, false, false, full_blend);
		anim_time						= layer->anm->anim_param().max_t / anm.speed;
		onMovementChanged				();
	}
	else
	{
		CObjectAnimator					animator;
		animator.Load					(anm.anm.c_str());
		animator.Play					(false);
		anim_time						= animator.anim_param().max_t / anm.speed;
	}

	if (state && anim_time > 0)
	{
		m_bStopAtEndAnimIsRunning		= true;
		m_dwMotionStartTm				= Device.dwTimeGlobal;
		m_dwMotionCurrTm				= m_dwMotionStartTm;
		m_dwMotionEndTm					= m_dwMotionStartTm + anim_time * 1000.f;
		m_startedMotionState			= state;
		if (HudItemData())
			m_current_anm				= anm.anm;
	}
}

void CHudItem::UpdateHudBonesVisibility()
{
	m_object->emitSignal				(sUpdateHudBonesVisibility());
}

void CHudItem::UpdateSlotsTransform()
{
	m_object->emitSignal				(sUpdateSlotsTransform());
}

void CHudItem::render_hud_mode()
{
	m_object->emitSignal				(sRenderHudMode());
}

bool CHudItem::motionPartPassed(float part) const
{
	float cur							= static_cast<float>(m_dwMotionCurrTm - m_dwMotionStartTm);
	float full							= static_cast<float>(m_dwMotionEndTm - m_dwMotionStartTm);
	return								(cur / full >= part);
}
