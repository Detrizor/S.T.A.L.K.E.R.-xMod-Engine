////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity_alive.h"
#include "inventory_item_impl.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "actor.h"
#include "actoreffector.h"
#include "level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "../xrphysics/mathutils.h"
#include "object_broker.h"
#include "player_hud.h"
#include "gamepersistent.h"
#include "effectorFall.h"
#include "debug_renderer.h"
#include "static_cast_checked.hpp"
#include "clsid_game.h"
#include "weaponBinocularsVision.h"
#include "ui/UIWindow.h"
#include "ui/UIXmlInit.h"
#include "Level_Bullet_Manager.h"
#include "cameralook.h"

#define WEAPON_REMOVE_TIME		60000
#define ROTATION_TIME			0.25f

CWeapon::CWeapon()
{
    SetState(eHidden);
    SetNextState(eHidden);
    m_sub_state = eSubstateReloadBegin;
    m_bTriStateReload = false;
    SetDefaults();

    m_Offset.identity();
    m_StrapOffset.identity();

    m_iAmmoCurrentTotal = 0;
    m_BriefInfo_CalcFrame = 0;

    iAmmoElapsed = 0;
    iMagazineSize = -1;
	m_ammoType = 0;

    eHandDependence = hdNone;

    m_zoom_params.m_fZoomRotationFactor = 0.f;

    m_pFlameParticles2 = NULL;
    m_sFlameParticles2 = NULL;

    m_fCurrentCartirdgeDisp = 1.f;

    m_strap_bone0 = 0;
    m_strap_bone1 = 0;
    m_StrapOffset.identity();
    m_strapped_mode = false;
    m_can_be_strapped = false;
    m_ef_main_weapon_type = u32(-1);
    m_ef_weapon_type = u32(-1);
    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_activation_speed_is_overriden = false;

	m_fLR_ShootingFactor = 0.f;
	m_fUD_ShootingFactor = 0.f;
	m_fBACKW_ShootingFactor = 0.f;
	m_fSafeModeRotateTime = 0.f;

	for (int i = 0; i < 2; i++)
		m_hud_offset[i].set(0.f, 0.f, 0.f);

	m_iADS = 0;
	m_bArmedMode = false;
	m_bHasAltAim = true;
	m_bArmedRelaxedSwitch = true;
}

CWeapon::~CWeapon()
{
}

void CWeapon::Hit(SHit* pHDS)
{
    inherited::Hit(pHDS);
}

void CWeapon::UpdateXForm()
{
    if (Device.dwFrame == dwXF_Frame)
        return;

    dwXF_Frame = Device.dwFrame;

    if (!H_Parent())
        return;

    // Get access to entity and its visual
    CEntityAlive*			E = smart_cast<CEntityAlive*>(H_Parent());

    if (!E)
       return;

    const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
    if (parent && parent->use_simplified_visual())
        return;

    if (parent->attached(this))
        return;

    IKinematics*			V = smart_cast<IKinematics*>	(E->Visual());
    VERIFY(V);

    // Get matrices
    int						boneL = -1, boneR = -1, boneR2 = -1;

    // this ugly case is possible in case of a CustomMonster, not a Stalker, nor an Actor
    E->g_WeaponBones(boneL, boneR, boneR2);

    if (boneR == -1)		return;

    if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
        boneL = boneR2;

    V->CalculateBones();
    Fmatrix& mL = V->LL_GetTransform(u16(boneL));
    Fmatrix& mR = V->LL_GetTransform(u16(boneR));
    // Calculate
    Fmatrix					mRes;
    Fvector					R, D, N;
    D.sub(mL.c, mR.c);

    if (fis_zero(D.magnitude()))
    {
        mRes.set(E->XFORM());
        mRes.c.set(mR.c);
    }
    else
    {
        D.normalize();
        R.crossproduct(mR.j, D);

        N.crossproduct(D, R);
        N.normalize();

        mRes.set(R, N, D, mR.c);
        mRes.mulA_43(E->XFORM());
    }

    UpdatePosition(mRes);
}

void CWeapon::UpdateFireDependencies_internal()
{
    if (Device.dwFrame != dwFP_Frame)
    {
        dwFP_Frame = Device.dwFrame;

        UpdateXForm();

        if (GetHUDmode())
        {
            HudItemData()->setup_firedeps(m_current_firedeps);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
        else
        {
            // 3rd person or no parent
            Fmatrix& parent = XFORM();
            Fvector& fp = vLoadedFirePoint;
            Fvector& fp2 = vLoadedFirePoint2;
            Fvector& sp = vLoadedShellPoint;

            parent.transform_tiny(m_current_firedeps.vLastFP, fp);
            parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
            parent.transform_tiny(m_current_firedeps.vLastSP, sp);

            m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
            parent.transform_dir(m_current_firedeps.vLastFD);

            m_current_firedeps.m_FireParticlesXForm.set(parent);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
    }
}

void CWeapon::ForceUpdateFireParticles()
{
    if (!GetHUDmode())
    {//update particlesXFORM real bullet direction
        if (!H_Parent())		return;

        Fvector					p, d;
        smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

        Fmatrix						_pxf;
        _pxf.k = d;
        _pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
        _pxf.j.crossproduct(_pxf.k, _pxf.i);
        _pxf.c = XFORM().c;

        m_current_firedeps.m_FireParticlesXForm.set(_pxf);
    }
}

void CWeapon::Load(LPCSTR section)
{
    inherited::Load(section);
    CShootingObject::Load(section);

    if (pSettings->line_exist(section, "flame_particles_2"))
        m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

    // load ammo classes
    m_ammoTypes.clear();
    LPCSTR				S = pSettings->r_string(section, "ammo_class");
    if (S && S[0])
    {
        string128		_ammoItem;
        int				count = _GetItemCount(S);
        for (int it = 0; it < count; ++it)
        {
            _GetItem(S, it, _ammoItem);
            m_ammoTypes.push_back(_ammoItem);
        }
	}

    iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");

    ////////////////////////////////////////////////////
    // дисперсия стрельбы

    //подбрасывание камеры во время отдачи
    u8 rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return", 1);
    cam_recoil.ReturnMode = (rm == 1);

    rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return_stop", 0);
    cam_recoil.StopReturn = (rm == 1);

    float temp_f = 0.0f;
    temp_f = pSettings->r_float(section, "cam_relax_speed");
    cam_recoil.RelaxSpeed = _abs(deg2rad(temp_f));
	//VERIFY(!fis_zero(cam_recoil.RelaxSpeed));
    if (fis_zero(cam_recoil.RelaxSpeed))
    {
        cam_recoil.RelaxSpeed = EPS_L;
    }

    cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed;
    if (pSettings->line_exist(section, "cam_relax_speed_ai"))
    {
        temp_f = pSettings->r_float(section, "cam_relax_speed_ai");
        cam_recoil.RelaxSpeed_AI = _abs(deg2rad(temp_f));
        VERIFY(!fis_zero(cam_recoil.RelaxSpeed_AI));
        if (fis_zero(cam_recoil.RelaxSpeed_AI))
        {
            cam_recoil.RelaxSpeed_AI = EPS_L;
        }
    }
    temp_f = pSettings->r_float(section, "cam_max_angle");
    cam_recoil.MaxAngleVert = _abs(deg2rad(temp_f));
	//VERIFY(!fis_zero(cam_recoil.MaxAngleVert));
    if (fis_zero(cam_recoil.MaxAngleVert))
    {
        cam_recoil.MaxAngleVert = EPS;
    }

    temp_f = pSettings->r_float(section, "cam_max_angle_horz");
    cam_recoil.MaxAngleHorz = _abs(deg2rad(temp_f));
    //VERIFY(!fis_zero(cam_recoil.MaxAngleHorz));
    if (fis_zero(cam_recoil.MaxAngleHorz))
    {
        cam_recoil.MaxAngleHorz = EPS;
    }

    temp_f = pSettings->r_float(section, "cam_step_angle_horz");
    cam_recoil.StepAngleHorz = deg2rad(temp_f);

    cam_recoil.DispersionFrac = _abs(READ_IF_EXISTS(pSettings, r_float, section, "cam_dispersion_frac", 0.7f));

    //подбрасывание камеры во время отдачи в режиме zoom ==> ironsight or scope
    //zoom_cam_recoil.Clone( cam_recoil ); ==== нельзя !!!!!!!!!!
    zoom_cam_recoil.RelaxSpeed = cam_recoil.RelaxSpeed;
    zoom_cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed_AI;
    zoom_cam_recoil.DispersionFrac = cam_recoil.DispersionFrac;
    zoom_cam_recoil.MaxAngleVert = cam_recoil.MaxAngleVert;
    zoom_cam_recoil.MaxAngleHorz = cam_recoil.MaxAngleHorz;
    zoom_cam_recoil.StepAngleHorz = cam_recoil.StepAngleHorz;

    zoom_cam_recoil.ReturnMode = cam_recoil.ReturnMode;
    zoom_cam_recoil.StopReturn = cam_recoil.StopReturn;

    if (pSettings->line_exist(section, "zoom_cam_relax_speed"))
    {
        zoom_cam_recoil.RelaxSpeed = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed")));
        VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed));
        if (fis_zero(zoom_cam_recoil.RelaxSpeed))
        {
            zoom_cam_recoil.RelaxSpeed = EPS_L;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_relax_speed_ai"))
    {
        zoom_cam_recoil.RelaxSpeed_AI = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed_ai")));
        VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed_AI));
        if (fis_zero(zoom_cam_recoil.RelaxSpeed_AI))
        {
            zoom_cam_recoil.RelaxSpeed_AI = EPS_L;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle"))
    {
        zoom_cam_recoil.MaxAngleVert = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleVert));
        if (fis_zero(zoom_cam_recoil.MaxAngleVert))
        {
            zoom_cam_recoil.MaxAngleVert = EPS;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle_horz"))
    {
        zoom_cam_recoil.MaxAngleHorz = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle_horz")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleHorz));
        if (fis_zero(zoom_cam_recoil.MaxAngleHorz))
        {
            zoom_cam_recoil.MaxAngleHorz = EPS;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_step_angle_horz"))
    {
        zoom_cam_recoil.StepAngleHorz = deg2rad(pSettings->r_float(section, "zoom_cam_step_angle_horz"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_frac"))
    {
        zoom_cam_recoil.DispersionFrac = _abs(pSettings->r_float(section, "zoom_cam_dispersion_frac"));
    }

	m_pdm.m_fPDM_disp_base			= pSettings->r_float(section, "PDM_disp_base");
    m_pdm.m_fPDM_disp_vel_factor	= pSettings->r_float(section, "PDM_disp_vel_factor");

    m_first_bullet_controller.load(section);
    fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");

    // modified by Peacemaker [17.10.08]
    //	misfireProbability			  = pSettings->r_float(section,"misfire_probability");
    //	misfireConditionK			  = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k",	1.0f);
    misfireStartCondition = pSettings->r_float(section, "misfire_start_condition");
    misfireEndCondition = READ_IF_EXISTS(pSettings, r_float, section, "misfire_end_condition", 0.f);
    misfireStartProbability = READ_IF_EXISTS(pSettings, r_float, section, "misfire_start_prob", 0.f);
    misfireEndProbability = pSettings->r_float(section, "misfire_end_prob");
    conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
    conditionDecreasePerQueueShot = READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot);

    vLoadedFirePoint = pSettings->r_fvector3(section, "fire_point");

    if (pSettings->line_exist(section, "fire_point2"))
        vLoadedFirePoint2 = pSettings->r_fvector3(section, "fire_point2");
    else
        vLoadedFirePoint2 = vLoadedFirePoint;

    // hands
    eHandDependence = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
    m_bIsSingleHanded = true;
    if (pSettings->line_exist(section, "single_handed"))
        m_bIsSingleHanded = !!pSettings->r_bool(section, "single_handed");
    //

    m_zoom_params.m_bZoomEnabled = !!pSettings->r_bool(section, "zoom_enabled");
	if (IsZoomEnabled())
		InitRotateTime ();

    if (pSettings->line_exist(section, "weapon_remove_time"))
        m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
    else
        m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;

    if (pSettings->line_exist(section, "auto_spawn_ammo"))
        m_bAutoSpawnAmmo = pSettings->r_bool(section, "auto_spawn_ammo");
    else
        m_bAutoSpawnAmmo = TRUE;
	
	m_zoom_params.m_bHideCrosshairInZoom = !!pSettings->r_bool(section, "zoom_hide_crosshair");

    Fvector			def_dof;
    def_dof.set(-1, -1, -1);
    m_zoom_params.m_ZoomDof = READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof", Fvector().set(-1, -1, -1));
    m_zoom_params.m_bZoomDofEnabled = !def_dof.similar(m_zoom_params.m_ZoomDof);

    m_zoom_params.m_ReloadDof = READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_dof", Fvector4().set(-1, -1, -1, -1));

    //Swartz: empty reload
    m_zoom_params.m_ReloadEmptyDof = READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_empty_dof", Fvector4().set(-1, -1, -1, -1));
    //-Swartz

    m_bHasTracers = !!READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
    m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

    string256						temp;
    for (int i = egdNovice; i < egdCount; ++i)
    {
        strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
        m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
    }

	// Added by Axel, to enable optional condition use on any item
	m_flags.set( FUsingCondition, READ_IF_EXISTS( pSettings, r_bool, section, "use_condition", TRUE ));

	m_fSafeModeRotateTime = READ_IF_EXISTS(pSettings, r_float, section, "weapon_relax_speed", 1.f);

	// Rezy safemode blend anms
	m_safemode_anm[0].name = READ_IF_EXISTS(pSettings, r_string, *hud_sect, "safemode_anm", nullptr);
	m_safemode_anm[1].name = READ_IF_EXISTS(pSettings, r_string, *hud_sect, "safemode_anm2", nullptr);
	m_safemode_anm[0].speed = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "safemode_anm_speed", 1.f);
	m_safemode_anm[1].speed = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "safemode_anm_speed2", 1.f);
	m_safemode_anm[0].power = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "safemode_anm_power", 1.f);
	m_safemode_anm[1].power = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "safemode_anm_power2", 1.f);

	m_shoot_shake_mat.identity();

	m_bHasAltAim = !!READ_IF_EXISTS(pSettings, r_bool, section, "has_alt_aim", TRUE);
	m_bArmedRelaxedSwitch = !!READ_IF_EXISTS(pSettings, r_bool, section, "armed_relaxed_switch", TRUE);
	m_bArmedMode = !m_bArmedRelaxedSwitch;
}

void CWeapon::LoadFireParams(LPCSTR section)
{
    cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "cam_dispersion"));
    cam_recoil.DispersionInc = 0.0f;

    if (pSettings->line_exist(section, "cam_dispersion_inc"))
    {
        cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "cam_dispersion_inc"));
    }

    zoom_cam_recoil.Dispersion = cam_recoil.Dispersion;
    zoom_cam_recoil.DispersionInc = cam_recoil.DispersionInc;

    if (pSettings->line_exist(section, "zoom_cam_dispersion"))
    {
        zoom_cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_inc"))
    {
        zoom_cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion_inc"));
    }

    CShootingObject::LoadFireParams(section);
};

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
    BOOL bResult					= inherited::net_Spawn(DC);
    CSE_Abstract					*e = (CSE_Abstract*) (DC);
    CSE_ALifeItemWeapon			    *E = smart_cast<CSE_ALifeItemWeapon*>(e);

    iAmmoElapsed = E->a_elapsed;
    m_ammoType = E->ammo_type;
    SetState(E->wpn_state);
    SetNextState(E->wpn_state);

	if (!m_ammoTypes[m_ammoType])
		m_ammoType = 0;

    m_DefaultCartridge.Load			(*m_ammoTypes[m_ammoType], m_ammoType);
    if (iAmmoElapsed)
    {
        m_fCurrentCartirdgeDisp		= m_DefaultCartridge.param_s.kDisp;
		SetAmmoElapsed				(iAmmoElapsed);
    }

    m_dwWeaponIndependencyTime = 0;

	cNameVisual_set(shared_str().printf("_w_%s", *cNameVisual()));

    return bResult;
}

u8 CWeapon::FindAmmoClass(LPCSTR section, bool set)
{
	for (u8 i = 0, i_e = u8(m_ammoTypes.size()); i < i_e; ++i)
	{
		if (m_ammoTypes[i] == section)
		{
			if (set)
				m_ammoType = i;
			return i + 1;
		}
	}
	return 0;
}

void CWeapon::PrepareCartridgeToShoot()
{
}

void CWeapon::ConsumeShotCartridge()
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor && pActor->unlimited_ammo())
		return;

	m_magazine.pop_back();
	--iAmmoElapsed;
	if (!m_magazine.empty())
		FindAmmoClass(*m_magazine.back().m_ammoSect, true);
}

void CWeapon::net_Destroy()
{
    inherited::net_Destroy();

    //удалить объекты партиклов
    StopFlameParticles();
    StopFlameParticles2();
    StopLight();
    Light_Destroy();

    while (m_magazine.size()) m_magazine.pop_back();
}

BOOL CWeapon::IsUpdating()
{
    bool bIsActiveItem = m_pInventory && m_pInventory->ActiveItem() == this;
    return bIsActiveItem || bWorking;// || IsPending() || getVisible();
}

void CWeapon::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);

    P.w_float_q8(GetCondition(), 0.0f, 1.0f);

    u8 need_upd = IsUpdating() ? 1 : 0;
    P.w_u8(need_upd);
    P.w_u16(u16(iAmmoElapsed));
    P.w_u8(0);		//--xd
    P.w_u8(m_ammoType);
    P.w_u8((u8) GetState());
    P.w_u8((u8) IsZoomed());
}

void CWeapon::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    float _cond;
    P.r_float_q8(_cond, 0.0f, 1.0f);
    SetCondition(_cond);

    u8 flags = 0;
    P.r_u8(flags);

    u16 ammo_elapsed = 0;
    P.r_u16(ammo_elapsed);

	P.r_u8(flags);

    u8 ammoType, wstate;
    P.r_u8(ammoType);
    P.r_u8(wstate);

    u8 Zoom;
    P.r_u8((u8) Zoom);

    if (H_Parent() && H_Parent()->Remote())
    {
        if (Zoom) OnZoomIn();
        else OnZoomOut();
    };
    switch (wstate)
    {
    case eFire:
    case eFire2:
    case eSwitch:
    case eReload:
    {
    }break;
    default:
    {
        if (ammoType >= m_ammoTypes.size())
            Msg("!! Weapon [%d], State - [%d]", ID(), wstate);
        else
        {
            m_ammoType = ammoType;
            SetAmmoElapsed(ammo_elapsed);
        }
    }break;
    }

    VERIFY((u32) iAmmoElapsed == m_magazine.size());
}

void CWeapon::save(NET_Packet &output_packet)
{
    inherited::save(output_packet);
	save_data(iAmmoElapsed, output_packet);
	save_data(m_ammoType, output_packet);		//--xd
	save_data(m_ammoType, output_packet);
	save_data(m_ammoType, output_packet);
    save_data(m_zoom_params.m_bIsZoomModeNow, output_packet);
	save_data(m_ammoType, output_packet);
}

void CWeapon::load(IReader &input_packet)
{
    inherited::load(input_packet);
	load_data(iAmmoElapsed, input_packet);
	load_data(m_ammoType, input_packet);
	load_data(m_ammoType, input_packet);
	load_data(m_ammoType, input_packet);
    load_data(m_zoom_params.m_bIsZoomModeNow, input_packet);

    if (m_zoom_params.m_bIsZoomModeNow)
        OnZoomIn();
    else
        OnZoomOut();

	load_data(m_ammoType, input_packet);
}

void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_WPN_STATE_CHANGE:{
        u8 state;
        P.r_u8(state);
        P.r_u8(m_sub_state);
        //			u8 NewAmmoType =
        P.r_u8();
        u8 AmmoElapsed = P.r_u8();
        u8 NextAmmo = P.r_u8();
        if (NextAmmo == undefined_ammo_type)
            m_set_next_ammoType_on_reload = undefined_ammo_type;
        else
            m_set_next_ammoType_on_reload = NextAmmo;

        if (OnClient()) SetAmmoElapsed(int(AmmoElapsed));
        OnStateSwitch(u32(state), GetState());
    }break;
    default:
        inherited::OnEvent(P, type);
    }
}

void CWeapon::shedule_Update(u32 dT)
{
    // Queue shrink
    //	u32	dwTimeCL		= Level().timeServer()-NET_Latency;
    //	while ((NET.size()>2) && (NET[1].dwTimeStamp<dwTimeCL)) NET.pop_front();

    // Inherited
    inherited::shedule_Update(dT);
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
    RemoveShotEffector();

    inherited::OnH_B_Independent(just_before_destroy);

    FireEnd();
    SetPending(FALSE);
    SwitchState(eHidden);

    m_strapped_mode = false;
    m_zoom_params.m_bIsZoomModeNow = false;
	UpdateXForm();
}

void CWeapon::OnH_A_Independent()
{
	m_fLR_ShootingFactor = 0.f;
	m_fUD_ShootingFactor = 0.f;
	m_fBACKW_ShootingFactor = 0.f;
    m_dwWeaponIndependencyTime = Level().timeServer();
    inherited::OnH_A_Independent();
    Light_Destroy();
	UpdateBonesVisibility();
};

void CWeapon::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
	UpdateBonesVisibility();
};

void CWeapon::OnActiveItem()
{
	UpdateBonesVisibility();
	m_BriefInfo_CalcFrame = 0;
	inherited::OnActiveItem();
}

void CWeapon::OnHiddenItem()
{
	m_BriefInfo_CalcFrame = 0;
	OnZoomOut();
	inherited::OnHiddenItem();
	m_set_next_ammoType_on_reload = undefined_ammo_type;
}

void CWeapon::SendHiddenItem()
{
    if (!CHudItem::object().getDestroy() && m_pInventory)
    {
        // !!! Just single entry for given state !!!
        NET_Packet		P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(eHiding));
        P.w_u8(u8(m_sub_state));
        P.w_u8(m_ammoType);
        P.w_u8(u8(iAmmoElapsed & 0xff));
        P.w_u8(m_set_next_ammoType_on_reload);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
        SetPending(TRUE);
    }
}

void CWeapon::OnH_B_Chield()
{
    m_dwWeaponIndependencyTime = 0;
    inherited::OnH_B_Chield();

    OnZoomOut();
    m_set_next_ammoType_on_reload = undefined_ammo_type;
}

extern u32 hud_adj_mode;
bool CWeapon::AllowBore()
{
	return ArmedMode();
}

void CWeapon::UpdateCL()
{
    inherited::UpdateCL();
    //подсветка от выстрела
    UpdateLight();

    //нарисовать партиклы
    UpdateFlameParticles();
    UpdateFlameParticles2();

    if ((GetNextState() == GetState()) && H_Parent() == Level().CurrentEntity())
    {
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor && !pActor->AnyMove() && this == pActor->inventory().ActiveItem())
        {
            if (hud_adj_mode == 0 &&
                GetState() == eIdle &&
                (Device.dwTimeGlobal - m_dw_curr_substate_time > 20000) &&
                !IsZoomed() &&
                g_player_hud->attached_item(1) == NULL)
            {
                if (AllowBore())
                    SwitchState(eBore);

                ResetSubStateTime();
            }
        }
    }
}

void CWeapon::renderable_Render()
{
	UpdateXForm();

    //нарисовать подсветку

    RenderLight();

    //если мы в режиме снайперки, то сам HUD рисовать не надо
	RenderHud((BOOL)need_renderable());

    inherited::renderable_Render();
}

void CWeapon::signal_HideComplete()
{
	m_fLR_ShootingFactor = 0.f;
	m_fUD_ShootingFactor = 0.f;
	m_fBACKW_ShootingFactor = 0.f;
    if (H_Parent())
        setVisible(FALSE);
    SetPending(FALSE);
}

void CWeapon::SetDefaults()
{
    SetPending(FALSE);

    m_flags.set(FUsingCondition, TRUE);
    bMisfire = false;
    m_zoom_params.m_bIsZoomModeNow = false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
    Position().set(trans.c);
    XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
    VERIFY(!fis_zero(DET(renderable.xform)));
}

#include "../../xrEngine/xr_input.h"
#include <fstream>
extern BOOL g_hud_adjusment_mode;
int hands_mode = 0;
bool CWeapon::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags)) return true;

    switch (cmd)
    {
    case kWPN_FIRE:
    {
        //если оружие чем-то занято, то ничего не делать
        {
            if (IsPending())
                return				false;

			if (flags&CMD_START)
			{
				if (ParentIsActor() && !ArmedMode())//--xd to fix GetCurrentHudOffsetIdx() == eRelaxed)
				{
					if (!IsZoomed() && !ArmedMode())
						SwitchArmedMode();
					return true;
				}

				FireStart();
			}
            else
                FireEnd();
        };
    }
    return true;

    case kWPN_ZOOM:
        if (IsZoomEnabled())
        {
			if (flags&CMD_START)
			{
				if (g_hud_adjusment_mode && IsZoomed())
					OnZoomOut();
				else if (!IsPending() && !IsZoomed())
					OnZoomIn();
            }
			else if (!g_hud_adjusment_mode)
				OnZoomOut();
            return true;
        }
        else
            return false;

    case kWPN_ZOOM_INC:
    case kWPN_ZOOM_DEC:
        if (flags&CMD_START)
        {
			if (cmd == kWPN_ZOOM_INC)  
				ZoomInc();
			else					
				ZoomDec();
			return true;
        }
        else
			return false;
    }

    return false;
}

int CWeapon::GetSuitableAmmoTotal(bool use_item_to_spawn) const
{
    int ae_count = iAmmoElapsed;
    if (!m_pInventory)
    {
        return ae_count;
    }

    //чтоб не делать лишних пересчетов
    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
    {
        return ae_count + m_iAmmoCurrentTotal;
    }
    m_BriefInfo_CalcFrame = Device.dwFrame;

    m_iAmmoCurrentTotal = 0;
    for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
    {
        m_iAmmoCurrentTotal += GetAmmoCount_forType(m_ammoTypes[i]);

        if (!use_item_to_spawn)
        {
            continue;
        }
        if (!inventory_owner().item_to_spawn())
        {
            continue;
        }
        m_iAmmoCurrentTotal += inventory_owner().ammo_in_box_to_spawn();
    }
    return ae_count + m_iAmmoCurrentTotal;
}

int CWeapon::GetAmmoCount(u8 ammo_type) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo_type < m_ammoTypes.size());

    return GetAmmoCount_forType(m_ammoTypes[ammo_type]);
}

int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type) const
{
    int res					= 0;
	for (TIItemContainer::iterator itb = m_pInventory->m_ruck.begin(), it_e = m_pInventory->m_ruck.end(); itb != it_e; ++itb)
    {
        CWeaponAmmo* pAmmo	= smart_cast<CWeaponAmmo*>(*itb);
        if (pAmmo && (pAmmo->cNameSect() == ammo_type))
            res				+= pAmmo->GetAmmoCount();
    }
    return					res;
}

float CWeapon::GetConditionMisfireProbability() const
{
    // modified by Peacemaker [17.10.08]
    //	if(GetCondition() > 0.95f)
    //		return 0.0f;
	float cond = GetConditionToWork();
	if (cond > misfireStartCondition)
        return 0.0f;
	if (cond < misfireEndCondition)
        return misfireEndProbability;
    //	float mis = misfireProbability+powf(1.f-GetCondition(), 3.f)*misfireConditionK;
    float mis = misfireStartProbability + (
		(misfireStartCondition - cond) *				// condition goes from 1.f to 0.f
        (misfireEndProbability - misfireStartProbability) /		// probability goes from 0.f to 1.f
        ((misfireStartCondition == misfireEndCondition) ?		// !!!say "No" to devision by zero
    misfireStartCondition :
                          (misfireStartCondition - misfireEndCondition))
                          );
    clamp(mis, 0.0f, 0.99f);
    return mis;
}

BOOL CWeapon::CheckForMisfire()
{
    if (OnClient()) return FALSE;

    float rnd = ::Random.randF(0.f, 1.f);
    float mp = GetConditionMisfireProbability();
    if (rnd < mp)
    {
        FireEnd();

        bMisfire = true;
        SwitchState(eMisfire);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CWeapon::IsMisfire() const
{
    return bMisfire;
}
void CWeapon::Reload()
{
    OnZoomOut();
}

void CWeapon::OnZoomIn()
{
	m_zoom_params.m_bIsZoomModeNow = true;
	g_player_hud->updateMovementLayerState();
}

void CWeapon::OnZoomOut()
{
    m_zoom_params.m_bIsZoomModeNow = false;
	if (ADS())
		SetADS(0);
	g_player_hud->updateMovementLayerState();
}

void CWeapon::SwitchState(u32 S)
{
    if (OnClient()) return;

#ifndef MASTER_GOLD
    if ( bDebug )
    {
        Msg("---Server is going to send GE_WPN_STATE_CHANGE to [%d], weapon_section[%s], parent[%s]",
            S, cNameSect().c_str(), H_Parent() ? H_Parent()->cName().c_str() : "NULL Parent");
    }
#endif // #ifndef MASTER_GOLD

    SetNextState(S);
    if (CHudItem::object().Local() && !CHudItem::object().getDestroy() && m_pInventory && OnServer())
    {
        // !!! Just single entry for given state !!!
        NET_Packet		P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(S));
        P.w_u8(u8(m_sub_state));
        P.w_u8(m_ammoType);
        P.w_u8(u8(iAmmoElapsed & 0xff));
        P.w_u8(m_set_next_ammoType_on_reload);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
    }
}

void CWeapon::OnMagazineEmpty()
{
    VERIFY((u32) iAmmoElapsed == m_magazine.size());
}

void CWeapon::reinit()
{
    CShootingObject::reinit();
    inherited::reinit();
}

void CWeapon::reload(LPCSTR section)
{
    CShootingObject::reload(section);
	inherited::reload(section);

    m_can_be_strapped = true;
    m_strapped_mode = false;

    if (pSettings->line_exist(section, "strap_bone0"))
        m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
    else
        m_can_be_strapped = false;

    if (pSettings->line_exist(section, "strap_bone1"))
        m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
    else
        m_can_be_strapped = false;

    {
        Fvector				pos, ypr;
        pos = pSettings->r_fvector3(section, "position");
        ypr = pSettings->r_fvector3(section, "orientation");
        ypr.mul(PI / 180.f);

        m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
        m_Offset.translate_over(pos);
    }

    m_StrapOffset = m_Offset;
    if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation"))
    {
        Fvector				pos, ypr;
        pos = pSettings->r_fvector3(section, "strap_position");
        ypr = pSettings->r_fvector3(section, "strap_orientation");
        ypr.mul(PI / 180.f);

        m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
        m_StrapOffset.translate_over(pos);
    }
    else
        m_can_be_strapped = false;

    m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", u32(-1));
    m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
}

void CWeapon::create_physic_shell()
{
    CPhysicsShellHolder::create_physic_shell();
}

bool CWeapon::ActivationSpeedOverriden(Fvector& dest, bool clear_override)
{
    if (m_activation_speed_is_overriden)
    {
        if (clear_override)
        {
            m_activation_speed_is_overriden = false;
        }

        dest = m_overriden_activation_speed;
        return							true;
    }

    return								false;
}

void CWeapon::SetActivationSpeedOverride(Fvector const& speed)
{
    m_overriden_activation_speed = speed;
    m_activation_speed_is_overriden = true;
}

void CWeapon::activate_physic_shell()
{
    UpdateXForm();
    CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
    CPhysicsShellHolder::setup_physic_shell();
}

int		g_iWeaponRemove = 1;

ALife::_TIME_ID	 CWeapon::TimePassedAfterIndependant()	const
{
    if (!H_Parent() && m_dwWeaponIndependencyTime != 0)
        return Level().timeServer() - m_dwWeaponIndependencyTime;
    else
        return 0;
}

bool CWeapon::can_kill() const
{
    if (GetSuitableAmmoTotal(true) || m_ammoTypes.empty())
        return				(true);

    return					(false);
}

CInventoryItem *CWeapon::can_kill(CInventory *inventory) const
{
    if (GetAmmoElapsed() || m_ammoTypes.empty())
        return				(const_cast<CWeapon*>(this));

    TIItemContainer::iterator I = inventory->m_all.begin();
    TIItemContainer::iterator E = inventory->m_all.end();
    for (; I != E; ++I)
    {
        CInventoryItem	*inventory_item = smart_cast<CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return			(inventory_item);
    }

    return					(0);
}

const CInventoryItem *CWeapon::can_kill(const xr_vector<const CGameObject*> &items) const
{
    if (m_ammoTypes.empty())
        return				(this);

    xr_vector<const CGameObject*>::const_iterator I = items.begin();
    xr_vector<const CGameObject*>::const_iterator E = items.end();
    for (; I != E; ++I)
    {
        const CInventoryItem	*inventory_item = smart_cast<const CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return			(inventory_item);
    }

    return					(0);
}

bool CWeapon::ready_to_kill() const
{
	//Alundaio
	const CInventoryOwner* io = smart_cast<const CInventoryOwner*>(H_Parent());
	if (!io)
		return false;

	if (io->inventory().ActiveItem() == NULL || io->inventory().ActiveItem()->object().ID() != ID())
		return false; 
	//-Alundaio
    return					(
        !IsMisfire() &&
        ((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) &&
        GetAmmoElapsed()
        );
}

void CWeapon::SetAmmoElapsed(int ammo_count)
{
    iAmmoElapsed = ammo_count;

    u32 uAmmo = u32(iAmmoElapsed);

    if (uAmmo != m_magazine.size())
    {
        if (uAmmo > m_magazine.size())
        {
            CCartridge			l_cartridge;
            l_cartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
            while (uAmmo > m_magazine.size())
                m_magazine.push_back(l_cartridge);
        }
        else
        {
            while (uAmmo < m_magazine.size())
                m_magazine.pop_back();
        }
    }
}

u32	CWeapon::ef_main_weapon_type() const
{
    VERIFY(m_ef_main_weapon_type != u32(-1));
    return	(m_ef_main_weapon_type);
}

u32	CWeapon::ef_weapon_type() const
{
    VERIFY(m_ef_weapon_type != u32(-1));
    return	(m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem(const shared_str& item_sect)
{
    return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end());
}

bool CWeapon::unlimited_ammo()
{
	if (m_pInventory)
	{
		return inventory_owner().unlimited_ammo() && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
	}
	else
		return false;

    return ((GameID() == eGameIDDeathmatch) &&
        m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited));
}

float CWeapon::GetMagazineWeight(const decltype(CWeapon::m_magazine)& mag) const
{
    float res					= 0.f;
    const char* last_type		= nullptr;
    float last_ammo_weight		= 0.f;
    for (auto& c : mag)
    {
        // Usually ammos in mag have same type, use this fact to improve performance
        if (last_type != c.m_ammoSect.c_str())
        {
            last_type			= c.m_ammoSect.c_str();
            last_ammo_weight	= c.Weight();
        }
        res						+= last_ammo_weight;
	}

    return res;
}

bool CWeapon::show_crosshair()
{
	return ArmedMode() || ADS() && !ZoomHideCrosshair();
}

bool CWeapon::show_indicators()
{
	return need_renderable();
}

float CWeapon::GetConditionToShow() const
{
    return	(GetCondition());//powf(GetCondition(),4.0f));
}

BOOL CWeapon::ParentMayHaveAimBullet()
{
    CObject* O = H_Parent();
    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    return EA->cast_actor() != 0;
}

BOOL CWeapon::ParentIsActor()
{
    CObject* O = H_Parent();
    if (!O)
        return FALSE;

    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    if (!EA)
        return FALSE;

    return EA->cast_actor() != 0;
}

extern u32 hud_adj_mode;

void CWeapon::debug_draw_firedeps()
{
#ifdef DEBUG
    if(hud_adj_mode==5||hud_adj_mode==6||hud_adj_mode==7)
    {
        CDebugRenderer			&render = Level().debug_renderer();

        if(hud_adj_mode==5)
            render.draw_aabb(get_LastFP(),	0.005f,0.005f,0.005f,D3DCOLOR_XRGB(255,0,0));

        if(hud_adj_mode==6)
            render.draw_aabb(get_LastFP2(),	0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,0,255));

        if(hud_adj_mode==7)
            render.draw_aabb(get_LastSP(),		0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,255,0));
    }
#endif // DEBUG
}

const float &CWeapon::hit_probability() const
{
    VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
    return					(m_hit_probability[g_SingleGameDifficulty]);
}

void CWeapon::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    m_BriefInfo_CalcFrame = 0;

    if (GetState() == eReload)
    {
        if (iAmmoElapsed == 0) //Swartz: re-written to use reload empty DOF
        {
            if (H_Parent() == Level().CurrentEntity() && !fsimilar(m_zoom_params.m_ReloadEmptyDof.w, -1.0f))
            {
                CActor* current_actor = smart_cast<CActor*>(H_Parent());
                if (current_actor)
                    current_actor->Cameras().AddCamEffector(xr_new<CEffectorDOF>(m_zoom_params.m_ReloadEmptyDof));
            }
        }
        else
        {
            if (H_Parent() == Level().CurrentEntity() && !fsimilar(m_zoom_params.m_ReloadDof.w, -1.0f))
            {
                CActor* current_actor = smart_cast<CActor*>(H_Parent());
                if (current_actor)
                    current_actor->Cameras().AddCamEffector(xr_new<CEffectorDOF>(m_zoom_params.m_ReloadDof));
            }
        }
	}
}

void CWeapon::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
}

void CWeapon::render_hud_mode()
{
	RenderLight();
}

bool CWeapon::MovingAnimAllowedNow()
{
    return !IsZoomed();
}

bool CWeapon::IsHudModeNow()
{
    return (HudItemData() != NULL);
}

void CWeapon::ZoomInc()
{
	if (IsZoomed())
		SetADS							(ADS() ? -ADS() : 1);
	else if (!ArmedMode())
		SwitchArmedMode					();
}

void CWeapon::ZoomDec()
{
	if (IsZoomed())
		SetADS							(ADS() ? 0 : -1);
	else if (ArmedMode())
		SwitchArmedMode					();
}

bool CWeapon::IsRotatingToZoom() const
{
	return !fEqual(m_zoom_params.m_fZoomRotationFactor, 1.f, EPS_S);
}

void CWeapon::SetADS(int mode)
{
	if (mode == -1 && !m_bHasAltAim)
		return;

	if (mode)
	{
		if (!ADS())
		{
			if (m_zoom_params.m_bZoomDofEnabled)
				GamePersistent().SetEffectorDOF(m_zoom_params.m_ZoomDof);

			if (GetHUDmode())
				GamePersistent().SetPickableEffectorDOF(true);
		}
	}
	else
	{
		GamePersistent().RestoreEffectorDOF();

		if (GetHUDmode())
			GamePersistent().SetPickableEffectorDOF(false);

		ResetSubStateTime();
	}
	m_iADS = mode;
}

void CWeapon::SwitchArmedMode()
{
	if (IsPending() || !m_bArmedRelaxedSwitch)
		return;

	bool new_state = !ArmedMode();
	m_bArmedMode = new_state;
	g_player_hud->OnMovementChanged(mcAnyMove);
	g_player_hud->updateMovementLayerState();
	SetPending(TRUE);
	if (!new_state && m_safemode_anm[1].name)
		PlayBlendAnm(m_safemode_anm[1].name, m_safemode_anm[1].speed, m_safemode_anm[1].power, false);
	else if (m_safemode_anm[0].name)
		PlayBlendAnm(m_safemode_anm[0].name, m_safemode_anm[0].speed, m_safemode_anm[0].power, false);
}

bool CWeapon::NeedBlendAnm()
{
	if (GetState() == eIdle && !ArmedMode() && HandSlot() == BOTH_HANDS_SLOT)
		return true;

	if (IsZoomed())
		return true;

	return inherited::NeedBlendAnm();
}

float CWeapon::GetControlInertionFactorBase() const
{
	return inherited::GetControlInertionFactor();
}

float CWeapon::GetControlInertionFactor() const
{
	float inertion = GetControlInertionFactorBase() - 1.f;
	if (IsZoomed());
	else if (ArmedMode())
		inertion *= .5f;
	else
		inertion *= .1f;
	return 1.f + inertion;
}

float CWeapon::CurrentZoomFactor() const
{
	return (float)abs(ADS());
}

void CWeapon::InitRotateTime()
{
	float base_zoom_rotate_time = pSettings->r_float("weapon_manager", "base_zoom_rotate_time");
	float handlilng_factor = HandlingToRotationTimeFactor.Calc(GetControlInertionFactor());
	m_zoom_params.m_fZoomRotateTime = base_zoom_rotate_time * handlilng_factor;
}