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
#include "cameralook.h"

#define WEAPON_REMOVE_TIME		60000
#define ROTATION_TIME			0.25f

CWeapon::CWeapon()
{
	SetState(eHidden);
	m_sub_state = eSubstateReloadBegin;
	m_bTriStateReload = false;
	SetDefaults();

	m_Offset.identity();
	m_StrapOffset.identity();

	m_iAmmoCurrentTotal = 0;
	m_BriefInfo_CalcFrame = 0;

	eHandDependence = hdNone;

	m_strap_bone0 = 0;
	m_strap_bone1 = 0;
	m_StrapOffset.identity();
	m_strapped_mode = false;
	m_can_be_strapped = false;
	m_ef_main_weapon_type = u32(-1);
	m_ef_weapon_type = u32(-1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CWeapon::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	inherited::sSyncData				(se_obj, save);
	if (save)
		smart_cast<CSE_ALifeItemWeapon*>(se_obj)->wpn_flags = static_cast<u8>(IsUpdating());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	UpdateXForm							();
	if (GetHUDmode())
	{
		auto hi							= HudItemData();
		hi->update						(false);
		
		Fvector fire_point				= m_fire_point;
		if (m_fire_bone != u16_max)
		{
			Fmatrix CR$ fire_mat		= hi->m_model->LL_GetTransform(m_fire_bone);
			fire_mat.transform_tiny		(fire_point);
		}
		else
		{
			Fmatrix CR$ root_mat		= Visual()->dcast_PKinematics()->LL_GetTransform(0);
			fire_point.sub				(root_mat.c);
		}
		
		Fvector shell_point				= m_shell_point;
		if (m_shell_bone != u16_max)
		{
			Fmatrix CR$ shell_mat		= hi->m_model->LL_GetTransform(m_shell_bone);
			shell_mat.transform_tiny	(shell_point);
		}
		else
		{
			Fmatrix CR$ root_mat		= Visual()->dcast_PKinematics()->LL_GetTransform(0);
			shell_point.sub				(root_mat.c);
		}
		
		Fmatrix ftrans					= static_cast<Fmatrix>(hi->m_transform);
		ftrans.transform_tiny			(vLastFP, fire_point);
		ftrans.transform_dir			(vLastFD, vForward);
		ftrans.transform_tiny			(vLastSP, shell_point);

		m_FireParticlesXForm.identity	();
		m_FireParticlesXForm.k.set		(vLastFD);
		Fvector::generate_orthonormal_basis_normalized	(m_FireParticlesXForm.k,
														m_FireParticlesXForm.j, 
														m_FireParticlesXForm.i);
	}
	else
	{
		Fvector fire_point				= m_fire_point;
		if (m_fire_bone != u16_max)
		{
			Fmatrix CR$ fire_mat		= Visual()->dcast_PKinematics()->LL_GetTransform(m_fire_bone);
			fire_mat.transform_tiny		(fire_point);
		}
		XFORM().transform_tiny			(vLastFP, fire_point);

		XFORM().transform_dir			(vLastFD, vForward);
		
		Fvector shell_point				= m_shell_point;
		if (m_shell_bone != u16_max)
		{
			Fmatrix CR$ shell_mat		= Visual()->dcast_PKinematics()->LL_GetTransform(m_shell_bone);
			shell_mat.transform_tiny	(shell_point);
		}
		XFORM().transform_tiny			(vLastSP, shell_point);

		m_FireParticlesXForm.set		(XFORM());
	}

	VERIFY								(_valid(vLastFP));
	VERIFY								(_valid(vLastFD));
	VERIFY								(_valid(vLastSP));
	VERIFY								(_valid(m_FireParticlesXForm));
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

		m_FireParticlesXForm.set(_pxf);
	}
}

void CWeapon::Load(LPCSTR section)
{
	inherited::Load(section);
	CShootingObject::Load(section);

	// load ammo classes
	LPCSTR S							= pSettings->r_string(section, "ammo_class");
	if (S && S[0])
	{
		string128						_ammoItem;
		for (int it = 0, count = _GetItemCount(S); it < count; ++it)
		{
			_GetItem					(S, it, _ammoItem);
			m_ammoTypes.emplace_back	(_ammoItem);
		}
		m_cartridge.Load				(m_ammoTypes[0].c_str());
	}

	////////////////////////////////////////////////////
	// дисперсия стрельбы

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

	// hands
	eHandDependence = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
	m_bIsSingleHanded = true;
	if (pSettings->line_exist(section, "single_handed"))
		m_bIsSingleHanded = !!pSettings->r_BOOL(section, "single_handed");
	//

	m_zoom_params.m_bZoomEnabled = !!pSettings->r_BOOL(section, "zoom_enabled");

	if (pSettings->line_exist(section, "weapon_remove_time"))
		m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
	else
		m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;

	if (pSettings->line_exist(section, "auto_spawn_ammo"))
		m_bAutoSpawnAmmo = pSettings->r_BOOL(section, "auto_spawn_ammo");
	else
		m_bAutoSpawnAmmo = TRUE;

	string256						temp;
	for (int i = egdNovice; i < egdCount; ++i)
	{
		strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
		m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
	}

	// Added by Axel, to enable optional condition use on any item
	m_flags.set( FUsingCondition, READ_IF_EXISTS( pSettings, r_BOOL, section, "use_condition", TRUE ));

	// Rezy safemode blend anms
	m_safemode_anm[0].load(hud_sect, "safemode_anm");
	m_safemode_anm[1].load(hud_sect, "safemode_anm2");

	m_bHasAltAim = !!READ_IF_EXISTS(pSettings, r_BOOL, section, "has_alt_aim", TRUE);
	m_bArmedRelaxedSwitch = !!READ_IF_EXISTS(pSettings, r_BOOL, section, "armed_relaxed_switch", TRUE);
	
	m_mechanic_recoil_pattern			= readRecoilPattern(section, "mechanic");
	m_layout_recoil_pattern				= readRecoilPattern(section, "layout");
	m_layout_accuracy_modifier			= readAccuracyModifier(section, "layout");

	if (pSettings->line_exist(section, "fire_bone"))
		m_fire_bone						= Visual()->dcast_PKinematics()->LL_BoneID(pSettings->r_string(section, "fire_bone"));
	if (pSettings->line_exist(section, "shell_bone"))
		m_shell_bone					= Visual()->dcast_PKinematics()->LL_BoneID(pSettings->r_string(section, "shell_bone"));
	
	m_fire_point						= pSettings->r_fvector3(section, "fire_point");
	m_shell_point						= pSettings->r_fvector3(section, "shell_point");
	m_grip_point						= pSettings->r_fvector3(section, "grip_point");
	m_Offset.translate_sub				(m_grip_point);
}

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
	BOOL bResult					= inherited::net_Spawn(DC);
	m_dwWeaponIndependencyTime		= 0;
	cNameVisual_set					(shared_str().printf("_w_%s", *cNameVisual()));
	return							bResult;
}

float CWeapon::get_wpn_pos_inertion_factor() const
{
	if (ADS())
		return							s_inertion_ads_factor;
	else if (IsZoomed())
		return							s_inertion_aim_factor;
	else if (ArmedMode())
		return							s_inertion_armed_factor;
	else
		return							s_inertion_relaxed_factor;
}

void CWeapon::net_Destroy()
{
	inherited::net_Destroy();

	//удалить объекты партиклов
	StopFlameParticles();
	StopLight();
	Light_Destroy();
}

BOOL CWeapon::IsUpdating()
{
	bool bIsActiveItem = m_pInventory && m_pInventory->ActiveItem() == this;
	return bIsActiveItem || bWorking;// || IsPending() || getVisible();
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
	RemoveShotEffector();
	inherited::OnH_B_Independent(just_before_destroy);
	FireEnd();
	m_strapped_mode = false;
	m_zoom_params.m_bIsZoomModeNow = false;
}

void CWeapon::OnH_A_Independent()
{
	m_dwWeaponIndependencyTime = Level().timeServer();
	inherited::OnH_A_Independent();
	Light_Destroy();
}

void CWeapon::OnActiveItem()
{
	m_BriefInfo_CalcFrame = 0;
	inherited::OnActiveItem();
}

void CWeapon::OnHiddenItem()
{
	m_BriefInfo_CalcFrame = 0;
	setAiming(false);
	inherited::OnHiddenItem();
}

void CWeapon::OnH_A_Chield()
{
	m_actor = H_Parent()->scast<CActor*>();
}

void CWeapon::OnH_B_Chield()
{
	m_dwWeaponIndependencyTime = 0;
	inherited::OnH_B_Chield();
	setAiming(false);
	m_actor = nullptr;
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

	if (H_Parent() == Level().CurrentEntity())
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
	__super::renderable_Render();

	//нарисовать подсветку
	RenderLight();

	//если мы в режиме снайперки, то сам HUD рисовать не надо
	RenderHud((BOOL)need_renderable());
}

void CWeapon::signal_HideComplete()
{
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

extern BOOL g_hud_adjusment_mode;
bool CWeapon::Action(u16 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;

	switch (cmd)
	{
	case kWPN_FIRE:
		if (IsPending() && GetState() != eFire)
			return false;

		if (flags&CMD_START)
			FireStart();
		else
			FireEnd();
		return true;

	case kWPN_ZOOM:
		if (!IsZoomEnabled())
			return false;

		if (flags&CMD_START)
		{
			if (g_hud_adjusment_mode && IsZoomed())
				setAiming(false);
			else if (!IsZoomed())
				setAiming(true);
		}
		else if (!g_hud_adjusment_mode)
			setAiming(false);
		return true;

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

int CWeapon::GetSuitableAmmoTotal() const
{
	if (unlimited_ammo())
		return int_max;

	int ae_count = GetAmmoElapsed();
	if (!m_pInventory)
		return ae_count;

	//чтоб не делать лишних пересчетов
	if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
	{
		return ae_count + m_iAmmoCurrentTotal;
	}
	m_BriefInfo_CalcFrame = Device.dwFrame;

	m_iAmmoCurrentTotal = 0;
	for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
		m_iAmmoCurrentTotal += GetAmmoCount_forType(m_ammoTypes[i]);
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

void CWeapon::setAiming(bool mode)
{
	m_zoom_params.m_bIsZoomModeNow		= mode;
	if (m_actor)
		m_actor->setZoomAimingMode		(mode);

	if (mode && !ArmedMode())
		SwitchArmedMode					();
	else if (!mode && ADS())
		setADS							(0);
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
	return (GetSuitableAmmoTotal() || m_ammoTypes.empty());
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

bool CWeapon::unlimited_ammo() const
{
	if (auto io = Parent->scast<CInventoryOwner*>())
		return io->unlimited_ammo();
	return false;
}

bool CWeapon::show_crosshair() const
{
	return !ArmedMode();
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
	CHudItem::OnStateSwitch				(S, oldState);
	m_BriefInfo_CalcFrame				= 0;
}

void CWeapon::render_hud_mode()
{
	inherited::render_hud_mode();
	RenderLight();
}

bool CWeapon::IsHudModeNow()
{
	return (HudItemData() != NULL);
}

void CWeapon::ZoomInc()
{
	if (IsZoomed())
		setADS							(ADS() ? -ADS() : 1);
	else if (!ArmedMode())
		SwitchArmedMode					();
}

void CWeapon::ZoomDec()
{
	if (IsZoomed())
		setADS							(ADS() ? 0 : -1);
	else if (ArmedMode())
		SwitchArmedMode					();
}

float CWeapon::readAccuracyModifier(LPCSTR section, LPCSTR line)
{
	return readAccuracyModifier(pSettings->r_string(section, line));
}

float CWeapon::readAccuracyModifier(LPCSTR type)
{
	return READ_IF_EXISTS(pSettings, r_float, "accuracy_modifiers", type, 1.f);
}

Fvector CWeapon::readRecoilPattern(LPCSTR section, LPCSTR line)
{
	return readRecoilPattern(pSettings->r_string(section, line));
}

Fvector CWeapon::readRecoilPattern(LPCSTR type)
{
	return READ_IF_EXISTS(pSettings, r_fvector3, "recoil_patterns", type, vOne);
}

Fvector CWeapon::getRecoilCamDelta()
{
	auto res = m_recoil_cam_delta;
	m_recoil_cam_delta = vZero;
	return res;
}

void CWeapon::setADS(int mode)
{
	if (mode == -1 && !HasAltAim())
		return;

	if (!mode)
		ResetSubStateTime();
	m_iADS = mode;
	if (m_actor)
		m_actor->setZoomADSMode(mode);
}

void CWeapon::SwitchArmedMode()
{
	if (m_actor && m_bArmedRelaxedSwitch)
		playBlendAnm(m_safemode_anm[m_actor->switchArmedMode()]);
}

float CWeapon::GetControlInertionFactor C$(bool full)
{
	float inertion						= sqrt((Weight() + s_inertion_baseline_weight) / s_inertion_baseline_weight) - 1.f;
	if (full)
		inertion						*= get_wpn_pos_inertion_factor();
	return								1.f + inherited::GetControlInertionFactor(full) * inertion;
}

float CWeapon::getZoom() const
{
	return								static_cast<float>(!!ADS());
}

bool CWeapon::ArmedMode() const
{
	if (m_bArmedRelaxedSwitch && m_actor)
		return							m_actor->getArmedMode();
	return								true;
}


float CWeapon::s_inertion_baseline_weight;
float CWeapon::s_inertion_ads_factor;
float CWeapon::s_inertion_aim_factor;
float CWeapon::s_inertion_armed_factor;
float CWeapon::s_inertion_relaxed_factor;

float CWeapon::s_recoil_kick_weight;
float CWeapon::s_recoil_tremble_weight;
float CWeapon::s_recoil_roll_weight;
float CWeapon::s_recoil_tremble_mean_dispersion;
float CWeapon::s_recoil_tremble_mean_change_chance;
float CWeapon::s_recoil_tremble_dispersion;
float CWeapon::s_recoil_kick_dispersion;
float CWeapon::s_recoil_roll_dispersion;

Fvector CWeapon::m_stock_recoil_pattern_absent;

void CWeapon::loadStaticData()
{
	s_inertion_baseline_weight			= pSettings->r_float("weapon_manager", "inertion_baseline_weight");
	s_inertion_ads_factor				= pSettings->r_float("weapon_manager", "inertion_ads_factor");
	s_inertion_aim_factor				= pSettings->r_float("weapon_manager", "inertion_aim_factor");
	s_inertion_armed_factor				= pSettings->r_float("weapon_manager", "inertion_armed_factor");
	s_inertion_relaxed_factor			= pSettings->r_float("weapon_manager", "inertion_relaxed_factor");
	
	s_recoil_kick_weight				= pSettings->r_float("weapon_manager", "recoil_kick_weight");
	s_recoil_tremble_weight				= pSettings->r_float("weapon_manager", "recoil_tremble_weight");
	s_recoil_roll_weight				= pSettings->r_float("weapon_manager", "recoil_roll_weight");
	s_recoil_tremble_mean_dispersion	= pSettings->r_float("weapon_manager", "recoil_tremble_mean_dispersion");
	s_recoil_tremble_mean_change_chance	= pSettings->r_float("weapon_manager", "recoil_tremble_mean_change_chance");
	s_recoil_tremble_dispersion			= pSettings->r_float("weapon_manager", "recoil_tremble_dispersion");
	s_recoil_kick_dispersion			= pSettings->r_float("weapon_manager", "recoil_kick_dispersion");
	s_recoil_roll_dispersion			= pSettings->r_float("weapon_manager", "recoil_roll_dispersion");

	m_stock_recoil_pattern_absent		= readRecoilPattern("absent");
}
