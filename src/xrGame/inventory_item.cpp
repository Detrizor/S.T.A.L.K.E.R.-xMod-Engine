////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch_script.h"
#include "inventory_item.h"
#include "inventory_item_impl.h"
#include "inventory.h"
//#include "Physics.h"
#include "physicsshellholder.h"
#include "entity_alive.h"
#include "Level.h"
#include "game_cl_base.h"
#include "Actor.h"
#include "string_table.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "object_broker.h"
#include "../xrEngine/igame_persistent.h"

#include "ai/trader/ai_trader.h"
#include "inventory.h"
#include "item_container.h"
#include "addon.h"
#include "addon_owner.h"
#include "inventory_item_amountable.h"
#include "item_usable.h"

#ifdef DEBUG
#	include "debug_renderer.h"
#endif

#define ITEM_REMOVE_TIME		30000

const float	CInventoryItem::m_fMaxRepairCondition = pSettings->r_float("miscellaneous", "max_repair_condition");

bool ItemCategory(const shared_str& section, LPCSTR cmp)
{
	return !xr_strcmp(pSettings->r_string(section, "category"), cmp);
}
bool ItemSubcategory(const shared_str& section, LPCSTR cmp)
{
	return !xr_strcmp(pSettings->r_string(section, "subcategory"), cmp);
}
bool ItemDivision(const shared_str& section, LPCSTR cmp)
{
	return !xr_strcmp(pSettings->r_string(section, "division"), cmp);
}

net_updateInvData* CInventoryItem::NetSync()
{
	if (!m_net_updateData)
		m_net_updateData = xr_new<net_updateInvData>();
	return m_net_updateData;
}

CInventoryItem::CInventoryItem(CGameObject* obj) : CModule(obj)
{
	m_net_updateData = NULL;
	m_flags.set(Fruck, TRUE);
	m_flags.set(FRuckDefault, TRUE);
	m_pInventory = NULL;

	SetDropManual(FALSE);

	m_flags.set(FCanTake, TRUE);
	m_can_trade = TRUE;
	m_flags.set(FCanTrade, m_can_trade);
	m_flags.set(FUsingCondition, FALSE);
	m_flags.set(FShowFullCondition, FALSE);
	m_fCondition = 1.0f;

	m_ItemCurrPlace.value = 0;
	m_ItemCurrPlace.type = eItemPlaceUndefined;
	m_ItemCurrPlace.base_slot_id = NO_ACTIVE_SLOT;
	m_ItemCurrPlace.hand_slot_id = NO_ACTIVE_SLOT;
	m_ItemCurrPlace.slot_id = NO_ACTIVE_SLOT;

	m_ItemCurrPlaceBackup.type = eItemPlaceUndefined;

	m_section_id = 0;
	m_flags.set(FIsHelperItem, FALSE);
	m_flags.set(FCanStack, TRUE);

	m_name = m_nameShort = m_Description = "";
	m_weight = m_volume = m_cost = m_upgrades_cost = 0.f;

	O.RegisterModule(this);
}

CInventoryItem::~CInventoryItem()
{
	delete_data(m_net_updateData);

#ifndef MASTER_GOLD
	bool B_GOOD = (!m_pInventory ||
		(std::find(m_pInventory->m_all.begin(), m_pInventory->m_all.end(), this) == m_pInventory->m_all.end()));
	if (!B_GOOD)
	{
		CObject* p = object().H_Parent();
		Msg("inventory ptr is [%s]", m_pInventory ? "not-null" : "null");
		if (p)
			Msg("parent name is [%s]", p->cName().c_str());

		Msg("! ERROR item_id[%d] H_Parent=[%s][%d] [%d]",
			object().ID(),
			p ? p->cName().c_str() : "none",
			p ? p->ID() : -1,
			Device.dwFrame);
	}
#endif // #ifndef MASTER_GOLD
}

void CInventoryItem::Load(LPCSTR section)
{
	CHitImmunity::LoadImmunities	(pSettings->r_string(section, "immunities_sect"), pSettings);

	ISpatial*						self = smart_cast<ISpatial*> (this);
	if (self)						self->spatial.type |= STYPE_VISIBLEFORAI;

	m_section_id._set				(section);

	LPCSTR sl						= pSettings->r_string(section, "slot");
	m_ItemCurrPlace.base_slot_id	= pSettings->r_u16("slot_ids", sl);
	sl								= READ_IF_EXISTS(pSettings, r_string, section, "hand_slot", "none");
	m_ItemCurrPlace.hand_slot_id	= pSettings->r_u16("slot_ids", sl);

	m_can_trade						= READ_IF_EXISTS(pSettings, r_bool, section, "can_trade", TRUE);
	m_flags.set						(FCanTake, READ_IF_EXISTS(pSettings, r_bool, section, "can_take", TRUE));
	m_flags.set						(FCanTrade, m_can_trade);
	m_flags.set						(FIsQuestItem, READ_IF_EXISTS(pSettings, r_bool, section, "quest_item", FALSE));
	m_flags.set						(FCanStack, READ_IF_EXISTS(pSettings, r_bool, section, "can_stack", TRUE));
	// Added by Axel, to enable optional condition use on any item
	m_flags.set						(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", FALSE));
	m_flags.set						(FShowFullCondition, READ_IF_EXISTS(pSettings, r_bool, section, "show_full_condition", FALSE));

	m_highlight_equipped			= !!READ_IF_EXISTS(pSettings, r_bool, section, "highlight_equipped", FALSE);
	m_icon_name						= READ_IF_EXISTS(pSettings, r_string, section, "icon_name", NULL);

	if (BaseSlot() != NO_ACTIVE_SLOT || HandSlot() != NO_ACTIVE_SLOT)
	{
		m_flags.set					(FRuckDefault, pSettings->r_bool(section, "default_to_ruck"));
		m_flags.set					(FAllowSprint, pSettings->r_bool(section, "sprint_allowed"));
		m_fControlInertionFactor	= pSettings->r_float(section, "control_inertion_factor");
	}

	m_category						= pSettings->r_string(section, "category");
	m_subcategory					= pSettings->r_string(section, "subcategory");
	m_division						= pSettings->r_string(section, "division");
	
	if (pSettings->line_exist(section, "inv_name"))
		m_name						= CStringTable().translate(pSettings->r_string(section, "inv_name"));
	m_nameShort						= (pSettings->line_exist(section, "inv_name_short")) ? CStringTable().translate(pSettings->r_string(section, "inv_name_short")) : m_name;
	m_Description					= (pSettings->line_exist(section, "description")) ? CStringTable().translate(pSettings->r_string(section, "description")) : "";
	m_weight						= pSettings->r_float(section, "inv_weight");
	R_ASSERT						(m_weight >= 0.f);
	m_volume						= pSettings->r_float(section, "inv_volume");
	R_ASSERT						(m_volume >= 0.f);
	m_cost							= ReadBaseCost(section);
	
	m_inv_icon_types				= !!READ_IF_EXISTS(pSettings, r_bool, section, "inv_icon_types", FALSE);
	SetInvIcon						();

	if (pSettings->line_exist			(section, "slots"))
		m_object->AddModule<CAddonOwner>();

	if (READ_IF_EXISTS(pSettings, r_bool, section, "addon", FALSE))
		m_object->AddModule<CAddon>();

	if (READ_IF_EXISTS(pSettings, r_bool, section, "amountable", FALSE))
		m_object->AddModule<CAmountable>();

	if (READ_IF_EXISTS(pSettings, r_bool, section, "usable", FALSE))
		m_object->AddModule<CUsable>();
}

float CInventoryItem::GetConditionToWork() const
{
	float condition		= m_fCondition / m_fMaxRepairCondition;
	return				(condition < 1.f) ? condition : 1.f;
}

void  CInventoryItem::ChangeCondition(float fDeltaCondition)
{
	m_fCondition += fDeltaCondition;
	clamp(m_fCondition, 0.f, 1.f);
}

void	CInventoryItem::Hit(SHit* pHDS)
{
	if (IsUsingCondition() == false) return;

	float hit_power = pHDS->damage();
	hit_power *= GetHitImmunity(pHDS->hit_type);

	ChangeCondition(-hit_power);
}

LPCSTR CInventoryItem::NameItem()
{
	return m_name.c_str();
}

LPCSTR CInventoryItem::NameShort() const
{
	return m_nameShort.c_str();
}

bool CInventoryItem::Useful() const
{
	return CanTake();
}

bool CInventoryItem::ActivateItem()
{
	return false;
}

void CInventoryItem::DeactivateItem()
{
}

void CInventoryItem::OnH_B_Independent(bool just_before_destroy)
{
	UpdateXForm();
	m_ItemCurrPlace.type = eItemPlaceUndefined;
}

void CInventoryItem::OnH_A_Independent()
{
	m_dwItemIndependencyTime = Level().timeServer();
	m_ItemCurrPlace.type = eItemPlaceUndefined;
	inherited::OnH_A_Independent();
}

void CInventoryItem::OnH_B_Chield()
{
	Level().RemoveObject_From_4CrPr(m_object);
}

void CInventoryItem::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
}
#ifdef DEBUG
extern	Flags32	dbg_net_Draw_Flags;
#endif

void CInventoryItem::UpdateCL()
{
#ifdef DEBUG
	if (bDebug){
		if (dbg_net_Draw_Flags.test(dbg_draw_invitem))
		{
			Device.seqRender.Remove(this);
			Device.seqRender.Add(this);
		}
		else
		{
			Device.seqRender.Remove(this);
		}
	}

#endif

}

void CInventoryItem::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_CHANGE_POS:{
		Fvector p;
		P.r_vec3(p);
		CPHSynchronize* pSyncObj = NULL;
		pSyncObj = object().PHGetSyncItem(0);
		if (!pSyncObj) return;
		SPHNetState state;
		pSyncObj->get_State(state);
		state.position = p;
		state.previous_position = p;
		pSyncObj->set_State(state);
		}break;
	}
}

/////////// network ///////////////////////////////
BOOL CInventoryItem::net_Spawn(CSE_Abstract* DC)
{
	VERIFY(!m_pInventory);

	m_flags.set(FInInterpolation, FALSE);
	m_flags.set(FInInterpolate, FALSE);
	//	m_bInInterpolation				= false;
	//	m_bInterpolate					= false;

	m_flags.set(Fuseful_for_NPC, TRUE);
	CSE_Abstract					*e = (CSE_Abstract*)(DC);
	CSE_ALifeObject					*alife_object = smart_cast<CSE_ALifeObject*>(e);
	if (alife_object)	{
		m_flags.set(Fuseful_for_NPC, alife_object->m_flags.test(CSE_ALifeObject::flUsefulForAI));
	}

	CSE_ALifeInventoryItem			*pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(e);
	if (!pSE_InventoryItem)			return TRUE;

	net_Spawn_install_upgrades(pSE_InventoryItem->m_upgrades);

	m_dwItemIndependencyTime = 0;

	m_just_after_spawn = true;
	m_activated = false;

	LPCSTR s_vis_name					= pSettings->r_string(m_section_id, "visual");
	if (xr_strcmp(s_vis_name, O.visual_name(e)))
	{
		O.cNameVisual_set				(s_vis_name);
		CSE_Visual* visual				= smart_cast<CSE_Visual*>(e);
		visual->set_visual				(s_vis_name);
	}

	return							TRUE;
}

void CInventoryItem::net_Destroy()
{
	if (m_pInventory)
		VERIFY(std::find(m_pInventory->m_all.begin(), m_pInventory->m_all.end(), this) == m_pInventory->m_all.end());
}

void CInventoryItem::save(NET_Packet &packet)
{
	packet.w_u16(m_ItemCurrPlace.value);
	packet.w_float(0.f);

	if (object().H_Parent())
	{
		packet.w_u8(0);
		return;
	}

	u8 _num_items = (u8)object().PHGetSyncItemsNumber();
	packet.w_u8(_num_items);
	object().PHSaveState(packet);
}

void CInventoryItem::load(IReader& packet)
{
	m_ItemCurrPlace.value = packet.r_u16();
	packet.r_float();

	if (!packet.r_u8())
		return;

	if (!object().PPhysicsShell()) {
		object().setup_physic_shell();
		object().PPhysicsShell()->Disable();
	}

	object().PHLoadState(packet);
	object().PPhysicsShell()->Disable();
}

void CInventoryItem::net_Import(NET_Packet& P)
{
	P.r_u8();
}

void CInventoryItem::net_Export(NET_Packet& P)
{
	P.w_u8(0);
}

void CInventoryItem::net_Import_PH_Params(NET_Packet& P, net_update_IItem& N, mask_inv_num_items& num_items)
{
	P.r_vec3(N.State.force);
	P.r_vec3(N.State.torque);
	P.r_vec3(N.State.position);
	P.r_float(N.State.quaternion.x);
	P.r_float(N.State.quaternion.y);
	P.r_float(N.State.quaternion.z);
	P.r_float(N.State.quaternion.w);

	N.State.enabled = num_items.mask & CSE_ALifeInventoryItem::inventory_item_state_enabled;
	if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_angular_null)) {
		N.State.angular_vel.x = P.r_float();
		N.State.angular_vel.y = P.r_float();
		N.State.angular_vel.z = P.r_float();
	}
	else
		N.State.angular_vel.set(0.f, 0.f, 0.f);

	if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_linear_null)) {
		N.State.linear_vel.x = P.r_float();
		N.State.linear_vel.y = P.r_float();
		N.State.linear_vel.z = P.r_float();
	}
	else
		N.State.linear_vel.set(0.f, 0.f, 0.f);

	N.State.previous_position = N.State.position;
	N.State.previous_quaternion = N.State.quaternion;
}

void CInventoryItem::net_Export_PH_Params(NET_Packet& P, SPHNetState& State, mask_inv_num_items&	num_items)
{
	P.w_vec3(State.force);
	P.w_vec3(State.torque);
	P.w_vec3(State.position);

	float					magnitude = _sqrt(State.quaternion.magnitude());
	if (fis_zero(magnitude)) {
		magnitude = 1;
		State.quaternion.x = 0.f;
		State.quaternion.y = 0.f;
		State.quaternion.z = 1.f;
		State.quaternion.w = 0.f;
	}

	P.w_float(State.quaternion.x);
	P.w_float(State.quaternion.y);
	P.w_float(State.quaternion.z);
	P.w_float(State.quaternion.w);

	if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_angular_null))
	{
		P.w_float(State.angular_vel.x);
		P.w_float(State.angular_vel.y);
		P.w_float(State.angular_vel.z);
	}

	if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_linear_null))
	{
		P.w_float(State.linear_vel.x);
		P.w_float(State.linear_vel.y);
		P.w_float(State.linear_vel.z);
	}
}

///////////////////////////////////////////////

void CInventoryItem::PH_A_CrPr()
{
	if (m_just_after_spawn)
	{
		VERIFY(object().Visual());
		IKinematics *K = object().Visual()->dcast_PKinematics();
		VERIFY(K);
		if (!object().PPhysicsShell())
		{
			Msg("! ERROR: PhysicsShell is NULL, object [%s][%d]", object().cName().c_str(), object().ID());
			VERIFY2(0, "physical shell is NULL");
			return;
		}
		if (!object().PPhysicsShell()->isFullActive())
		{
			K->CalculateBones_Invalidate();
			K->CalculateBones(TRUE);
		}
		object().PPhysicsShell()->GetGlobalTransformDynamic(&object().XFORM());
		K->CalculateBones_Invalidate();
		K->CalculateBones(TRUE);
#if	0
		Fbox bb = BoundingBox();
		DBG_OpenCashedDraw();
		Fvector c, r, p;
		bb.get_CD(c, r);
		XFORM().transform_tiny(p, c);
		DBG_DrawAABB(p, r, D3DCOLOR_XRGB(255, 0, 0));
		//PPhysicsShell()->XFORM().transform_tiny(c);
		Fmatrix mm;
		PPhysicsShell()->GetGlobalTransformDynamic(&mm);
		mm.transform_tiny(p, c);
		DBG_DrawAABB(p, r, D3DCOLOR_XRGB(0, 255, 0));
		DBG_ClosedCashedDraw(50000);
#endif
		object().spatial_move();
		m_just_after_spawn = false;

		VERIFY(!OnServer());

		object().PPhysicsShell()->get_ElementByStoreOrder(0)->Fix();
		object().PPhysicsShell()->SetIgnoreStatic();
	}
}

void CInventoryItem::Interpolate()
{
	net_updateInvData* p = NetSync();
	CPHSynchronize* pSyncObj = object().PHGetSyncItem(0);

	//simple linear interpolation...
	if (!object().H_Parent() &&
		object().getVisible() &&
		object().m_pPhysicsShell &&
		!OnServer() &&
		p->NET_IItem.size())
	{
		SPHNetState newState = p->NET_IItem.front().State;

		if (p->NET_IItem.size() >= 2)
		{

			float ret_interpolate = interpolate_states(p->NET_IItem.front(), p->NET_IItem.back(), newState);
			//Msg("Interpolation factor is %0.4f", ret_interpolate);
			//Msg("Current position is: x = %3.3f, y = %3.3f, z = %3.3f", newState.position.x, newState.position.y, newState.position.z);
			if (ret_interpolate >= 1.f)
			{
				p->NET_IItem.pop_front();
				if (m_activated)
				{
#ifdef DEBUG
					Msg("Deactivating object [%d] after interpolation finish", object().ID());
#endif // #ifdef DEBUG
					object().processing_deactivate();
					m_activated = false;
				}
			}
		}
		pSyncObj->set_State(newState);
	}
}
float CInventoryItem::interpolate_states(net_update_IItem const & first, net_update_IItem const & last, SPHNetState & current)
{
	float ret_val = 0.f;
	u32 CurTime = Device.dwTimeGlobal;

	if (CurTime == last.dwTimeStamp)
		return 0.f;

	float factor = float(CurTime - last.dwTimeStamp) / float(last.dwTimeStamp - first.dwTimeStamp);

	ret_val = factor;
	if (factor > 1.f)
	{
		factor = 1.f;
	}
	else if (factor < 0.f)
	{
		factor = 0.f;
	}

	current.position.x = first.State.position.x + (factor * (last.State.position.x - first.State.position.x));
	current.position.y = first.State.position.y + (factor * (last.State.position.y - first.State.position.y));
	current.position.z = first.State.position.z + (factor * (last.State.position.z - first.State.position.z));
	current.previous_position = current.position;

	current.quaternion.slerp(first.State.quaternion, last.State.quaternion, factor);
	current.previous_quaternion = current.quaternion;
	return ret_val;
}


void CInventoryItem::reload(LPCSTR section)
{
	inherited::reload(section);
}

void CInventoryItem::reinit()
{
	m_pInventory = NULL;
	m_ItemCurrPlace.type = eItemPlaceUndefined;
}

bool CInventoryItem::can_kill() const
{
	return				(false);
}

CInventoryItem *CInventoryItem::can_kill(CInventory *inventory) const
{
	return				(0);
}

const CInventoryItem *CInventoryItem::can_kill(const xr_vector<const CGameObject*> &items) const
{
	return				(0);
}

CInventoryItem *CInventoryItem::can_make_killing(const CInventory *inventory) const
{
	return				(0);
}

bool CInventoryItem::ready_to_kill() const
{
	return				(false);
}

void CInventoryItem::activate_physic_shell()
{
	CEntityAlive*	E = smart_cast<CEntityAlive*>(object().H_Parent());
	if (!E) {
		on_activate_physic_shell();
		return;
	};

	UpdateXForm();

	object().CPhysicsShellHolder::activate_physic_shell();
}

void CInventoryItem::UpdateXForm()
{
	if (0 == object().H_Parent())	return;

	// Get access to entity and its visual
	CEntityAlive*	E = smart_cast<CEntityAlive*>(object().H_Parent());
	if (!E) return;

	if (E->cast_base_monster()) return;

	const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
	if (parent && parent->use_simplified_visual())
		return;

	if (parent->attached(this))
		return;

	R_ASSERT(E);
	IKinematics*	V = smart_cast<IKinematics*>	(E->Visual());
	VERIFY(V);

	// Get matrices
	int						boneL = -1, boneR = -1, boneR2 = -1;
	E->g_WeaponBones(boneL, boneR, boneR2);
	if (boneR == -1)	return;
	//	if ((HandDependence() == hd1Hand) || (STATE == eReload) || (!E->g_Alive()))
	//		boneL = boneR2;
#pragma todo("TO ALL: serious performance problem")
	V->CalculateBones();
	Fmatrix& mL = V->LL_GetTransform(u16(boneL));
	Fmatrix& mR = V->LL_GetTransform(u16(boneR));
	// Calculate
	Fmatrix			mRes;
	Fvector			R, D, N;
	D.sub(mL.c, mR.c);	D.normalize_safe();

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

	//	UpdatePosition	(mRes);
	object().Position().set(mRes.c);
}

DLL_Pure *CInventoryItem::_construct()
{
	m_object = smart_cast<CPhysicsShellHolder*>(this);
	VERIFY(m_object);
	return		(inherited::_construct());
}

ALife::_TIME_ID	 CInventoryItem::TimePassedAfterIndependant()	const
{
	if (!object().H_Parent() && m_dwItemIndependencyTime != 0)
		return Level().timeServer() - m_dwItemIndependencyTime;
	else
		return 0;
}

bool CInventoryItem::CanTrade() const
{
	bool res	= (m_pInventory) ? inventory_owner().AllowItemToTrade(m_section_id, m_ItemCurrPlace) : true;
	return		(res && m_flags.test(FCanTrade) && !IsQuestItem());
}

Frect CInventoryItem::GetKillMsgRect() const
{
	float x, y, w, h;

	x = READ_IF_EXISTS(pSettings, r_float, m_object->cNameSect(), "kill_msg_x", 0.0f);
	y = READ_IF_EXISTS(pSettings, r_float, m_object->cNameSect(), "kill_msg_y", 0.0f);
	w = READ_IF_EXISTS(pSettings, r_float, m_object->cNameSect(), "kill_msg_width", 0.0f);
	h = READ_IF_EXISTS(pSettings, r_float, m_object->cNameSect(), "kill_msg_height", 0.0f);

	return Frect().set(x, y, w, h);
}

Frect CInventoryItem::GetIconRect() const
{
	return m_inv_icon;
}

Irect CInventoryItem::GetUpgrIconRect() const
{
	u32 x, y, w, h;

	x = READ_IF_EXISTS(pSettings, r_u32, m_object->cNameSect(), "upgr_icon_x", 0);
	y = READ_IF_EXISTS(pSettings, r_u32, m_object->cNameSect(), "upgr_icon_y", 0);
	w = READ_IF_EXISTS(pSettings, r_u32, m_object->cNameSect(), "upgr_icon_width", 0);
	h = READ_IF_EXISTS(pSettings, r_u32, m_object->cNameSect(), "upgr_icon_height", 0);

	return Irect().set(x, y, w, h);
}

bool CInventoryItem::IsNecessaryItem(CInventoryItem* item)
{
	return IsNecessaryItem(item->object().cNameSect());
};

BOOL CInventoryItem::IsInvalid() const
{
	return object().getDestroy() || GetDropManual();
}

u16 CInventoryItem::object_id()const
{
	return object().ID();
}

u16 CInventoryItem::parent_id() const
{
	return (object().H_Parent()) ? object().H_Parent()->ID() : u16(-1);
}

void CInventoryItem::SetDropManual(BOOL val)
{
	m_flags.set(FdropManual, val);
}

bool CInventoryItem::has_network_synchronization() const
{
	return false;
}

bool CInventoryItem::CanStack() const
{
	return (m_flags.test(FCanStack) > 0);
}

bool CInventoryItem::InHands() const
{
	CObject* parent = object().H_Parent();
	if (!parent)
		return false;
	CInventoryOwner* io = smart_cast<CInventoryOwner*>(parent);
	return (io) ? m_pInventory->InHands(const_cast<PIItem>(this)) : false;
}

u32 CInventoryItem::ReadBaseCost(LPCSTR section)
{
	float							cost;
	if (pSettings->line_exist("costs", section))
		cost						= pSettings->r_float("costs", section);
	else
		cost						= pSettings->r_float(section, "cost");
	if (pSettings->line_exist(section, "cost_factor"))
		cost						*= pSettings->r_float(section, "cost_factor");
	return							(u32)round(cost);
}

void CInventoryItem::ReadIcon(Frect& destination, LPCSTR section, u8 type, u8 idx)
{
	Fvector4 icon					= pSettings->r_fvector4(section, "inv_icon");
	float x							= icon.x + type * icon.z;
	float y							= icon.y + idx * pSettings->r_u16(section, "icon_index_step");
	destination.set					(x, y, x + icon.z, y + icon.w);
}

void CInventoryItem::SetInvIconType(u8 type)
{
	if (m_inv_icon_types)
	{
		m_inv_icon_type				= type;
		SetInvIcon					();
	}
}

void CInventoryItem::SetInvIconIndex(u8 idx)
{
	m_inv_icon_index				= idx;
	SetInvIcon						();
}

void CInventoryItem::SetInvIcon()
{
	ReadIcon						(m_inv_icon, *m_section_id, m_inv_icon_type, m_inv_icon_index);
}

bool CInventoryItem::Category C$(LPCSTR cmpc, LPCSTR cmps, LPCSTR cmpd)
{
	return (cmpc[0] == '*' || m_category == cmpc)
		&& (cmps[0] == '*' || m_subcategory == cmps)
		&& (cmpd[0] == '*' || m_division == cmpd);
}

shared_str CInventoryItem::Section(bool full) const
{
	return (full) ? shared_str().printf("%s_%d", *m_section_id, m_inv_icon_index) : m_section_id;
}

float CInventoryItem::Price() const
{
	return								Cost() * sqrt(GetCondition()) + m_upgrades_cost;
}

float CInventoryItem::GetAmount() const
{
	float res							= O.Aboba(eGetAmount);
	return								(res != flt_max) ? res : 1.f;
}

float CInventoryItem::GetFill() const
{
	float res							= O.Aboba(eGetFill);
	return								(res != flt_max) ? res : 1.f;
}

float CInventoryItem::GetBar() const
{
	float res							= O.Aboba(eGetBar);
	return								(res != flt_max) ? res : (fLess(GetCondition(), 1.f) ? GetCondition() : -1.f);
}

float CInventoryItem::Weight() const
{
	return								O.Aboba(eWeight);
}

float CInventoryItem::Volume() const
{
	return								O.Aboba(eVolume);
}

float CInventoryItem::Cost() const
{
	return								O.Aboba(eCost);
}

void CInventoryItem::Transfer(u16 id) const
{
	O.transfer							(id);
}

float CInventoryItem::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
	case eWeight:
		return						m_weight;
	case eVolume:
		return						m_volume;
	case eCost:
		return						m_cost;
	case eSyncData:
	{
		auto se_obj					= (CSE_ALifeDynamicObject*)data;
		auto m						= se_obj->getModule<CSE_ALifeModuleInventoryItem>(!!param);
		auto se_item				= smart_cast<CSE_ALifeItem*>(se_obj);
		if (param)
		{
			se_item->m_fCondition	= m_fCondition;
			m->m_icon_index			= m_inv_icon_index;
		}
		else
		{
			m_fCondition			= se_item->m_fCondition;
			if (m)
				SetInvIconIndex		(m->m_icon_index);
		}
	}
	}

	return								CModule::aboba(type, data, param);
}

bool CInventoryItem::tryCustomUse()
{
	if (auto usable = cast<CUsable*>())
	{
		int i							= 0;
		while (usable->getAction(++i))
			if (usable->performAction(i))
				return					true;
	}

	if (auto addon = cast<CAddon*>())
	{
		if (auto active_item = Actor()->inventory().ActiveItem())
		{
			if (auto ao = active_item->cast<CAddonOwner*>())
			{
				if (addon->tryAttach(ao))
					return				true;

				for (auto s : ao->AddonSlots())
					for (auto a : s->addons)
						if (auto sub_ao = a->cast<CAddonOwner*>())
							if (addon->tryAttach(sub_ao))
								return	true;
			}
		}
	}
	return								false;
}
