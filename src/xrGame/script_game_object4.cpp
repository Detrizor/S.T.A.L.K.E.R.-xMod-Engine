////////////////////////////////////////////////////////////////////////////
// script_game_object_trader.�pp :	������� ��� �������� � ���������
//////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"

#include "script_zone.h"
#include "ai/trader/ai_trader.h"

#include "ai_space.h"
#include "alife_simulator.h"

#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"

#include "sight_manager_space.h"
#include "sight_control_action.h"
#include "sight_manager.h"
#include "inventoryBox.h"
#include "ZoneCampfire.h"
#include "physicobject.h"
#include "artefact.h"
#include "stalker_sound_data.h"
#include "level_changer.h"

#include "eatable_item.h"
#include "car.h"
#include "helicopter.h"
#include "actor.h"
#include "customoutfit.h"
#include "customzone.h"
#include "ai\monsters\basemonster\base_monster.h"
#include "torch.h"
#include "GrenadeLauncher.h"
#include "searchlight.h"
#include "WeaponAmmo.h"
#include "grenade.h"
#include "BottleItem.h"
#include "WeaponAutomaticShotgun.h"
#include "inventory_item_amountable.h"
#include "item_container.h"
#include "Magazine.h"

#include "scope.h"
#include "silencer.h"
#include "addon.h"
#include "item_usable.h"
#include "foldable.h"
#include "artefact_module.h"

class CWeapon;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CScriptGameObject::is_body_turning() const
{
	CCustomMonster		*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CGameObject : cannot access class member is_turning!");
		return			(false);
	}

	CAI_Stalker			*stalker = smart_cast<CAI_Stalker*>(monster);
	if (!stalker)
		return			(!fis_zero(angle_difference(monster->movement().body_orientation().target.yaw, monster->movement().body_orientation().current.yaw)));
	else
		return			(!fis_zero(angle_difference(stalker->movement().head_orientation().target.yaw, stalker->movement().head_orientation().current.yaw)) || !fis_zero(angle_difference(monster->movement().body_orientation().target.yaw, monster->movement().body_orientation().current.yaw)));
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

u32	CScriptGameObject::add_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member add!");
		return					(0);
	}
	else
		return					(monster->sound().add(prefix, max_count, type, priority, mask, internal_type, bone_name));
}

u32	CScriptGameObject::add_combat_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name)
{
	CAI_Stalker* const stalker = smart_cast<CAI_Stalker*>(&object());
	if (!stalker)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member add!");
		return					(0);
	}
	else
		return					(stalker->sound().add(prefix, max_count, type, priority, mask, internal_type, bone_name, xr_new<CStalkerSoundData>(stalker)));
}

u32	CScriptGameObject::add_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type)
{
	return						(add_sound(prefix, max_count, type, priority, mask, internal_type, "bip01_head"));
}

void CScriptGameObject::remove_sound(u32 internal_type)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member add!");
	else
		monster->sound().remove(internal_type);
}

void CScriptGameObject::set_sound_mask(u32 sound_mask)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member set_sound_mask!");
	else
	{
		CEntityAlive			*entity_alive = smart_cast<CEntityAlive*>(monster);
		if (entity_alive)
		{
			VERIFY2(entity_alive->g_Alive(), "Stalkers talk after death??? Say why??");
		}
		monster->sound().set_sound_mask(sound_mask);
	}
}

void CScriptGameObject::play_sound(u32 internal_type)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type, max_start_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type, max_start_time, min_start_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time, min_stop_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time, u32 id)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CSoundPlayer : cannot access class member play!");
	else
		monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time, min_stop_time, id);
}

int  CScriptGameObject::active_sound_count(bool only_playing)
{
	CCustomMonster				*monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CGameObject : cannot access class member active_sound_count!");
		return								(-1);
	}
	else
		return								(monster->sound().active_sound_count(only_playing));
}

int CScriptGameObject::active_sound_count()
{
	return									(active_sound_count(false));
}

bool CScriptGameObject::wounded() const
{
	const CAI_Stalker			*stalker = smart_cast<const CAI_Stalker *>(&object());
	if (!stalker)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CAI_Stalker : cannot access class member wounded!");
		return					(false);
	}

	return						(stalker->wounded());
}

void CScriptGameObject::wounded(bool value)
{
	CAI_Stalker					*stalker = smart_cast<CAI_Stalker *>(&object());
	if (!stalker)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CAI_Stalker : cannot access class member wounded!");
		return;
	}

	stalker->wounded(value);
}

CSightParams CScriptGameObject::sight_params()
{
	CAI_Stalker						*stalker = smart_cast<CAI_Stalker*>(&object());
	if (!stalker)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CAI_Stalker : cannot access class member sight_params!");

		CSightParams				result;
		result.m_object = 0;
		result.m_vector = Fvector().set(flt_max, flt_max, flt_max);
		result.m_sight_type = SightManager::eSightTypeDummy;
		return						(result);
	}

	const CSightControlAction		&action = stalker->sight().current_action();
	CSightParams					result;
	result.m_sight_type = action.sight_type();
	result.m_object = action.object_to_look() ? action.object_to_look()->lua_game_object() : 0;
	result.m_vector = action.vector3d();
	return							(result);
}

bool CScriptGameObject::critically_wounded()
{
	CCustomMonster						*custom_monster = smart_cast<CCustomMonster*>(&object());
	if (!custom_monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CCustomMonster : cannot access class member critically_wounded!");
		return							(false);
	}

	return								(custom_monster->critically_wounded());
}

bool CScriptGameObject::IsInvBoxEmpty()
{
	MContainer* ib = object().getModule<MContainer>();
	if (!ib)
		return			(false);
	else
		return			ib->Empty();
}

bool CScriptGameObject::inv_box_closed(bool status, LPCSTR reason)
{
	CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
	if (!ib)
	{
		return			false;
	}
	else
	{
		ib->set_closed(status, reason);
		return			true;
	}
}

bool CScriptGameObject::inv_box_closed_status()
{
	CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
	if (!ib)
	{
		return			false;
	}
	else
	{
		return			ib->closed();
	}
}

bool CScriptGameObject::inv_box_can_take(bool status)
{
	CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
	if (!ib)
	{
		return			false;
	}
	else
	{
		ib->set_can_take(status);
		return			true;
	}
}

bool CScriptGameObject::inv_box_can_take_status()
{
	CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
	if (!ib)
	{
		return			false;
	}
	else
	{
		return			ib->can_take();
	}
}

CZoneCampfire* CScriptGameObject::get_campfire()
{
	return smart_cast<CZoneCampfire*>(&object());
}

CArtefact* CScriptGameObject::get_artefact()
{
	return smart_cast<CArtefact*>(&object());
}

CPhysicObject* CScriptGameObject::get_physics_object()
{
	return smart_cast<CPhysicObject*>(&object());
}

void CScriptGameObject::enable_level_changer(bool b)
{
	CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
	if (lch)
		lch->EnableLevelChanger(b);
}
bool CScriptGameObject::is_level_changer_enabled()
{
	CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
	if (lch)
		return lch->IsLevelChangerEnabled();
	return false;
}

void CScriptGameObject::set_level_changer_invitation(LPCSTR str)
{
	CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
	if (lch)
		lch->SetLEvelChangerInvitationStr(str);
}

void CScriptGameObject::start_particles(LPCSTR pname, LPCSTR bone)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(&object());
	if (!PP)	return;

	IKinematics* K = smart_cast<IKinematics*>(object().Visual());
	if (!K)
		return;
	
	//R_ASSERT(K);

	u16 play_bone = K->LL_BoneID(bone);
	R_ASSERT(play_bone != BI_NONE);
	if (K->LL_GetBoneVisible(play_bone))
		PP->StartParticles(pname, play_bone, Fvector().set(0, 1, 0), 9999);
	else
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "Cant start particles, bone [%s] is not visible now", bone);
}

void CScriptGameObject::stop_particles(LPCSTR pname, LPCSTR bone)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(&object());
	if (!PP)	return;

	IKinematics* K = smart_cast<IKinematics*>(object().Visual());
	if (!K)
		return;
	
	//R_ASSERT(K);

	u16 play_bone = K->LL_BoneID(bone);
	R_ASSERT(play_bone != BI_NONE);

	if (K->LL_GetBoneVisible(play_bone))
		PP->StopParticles(9999, play_bone, true);
	else
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "Cant stop particles, bone [%s] is not visible now", bone);
}

//AVO: directly set entity health instead of going throuhg normal health property which operates on delta
void CScriptGameObject::SetHealthEx(float hp)
{
	CEntity *obj = smart_cast<CEntity*>(&object());
	if (!obj) return;
	clamp(hp, -0.01f, 1.0f);
	obj->SetfHealth(hp);
}
//-AVO

void CScriptGameObject::ActorSetHealth(float h)
{
	CActor* actor = smart_cast<CActor*>(&object());
	if (!actor)
		return;
	actor->SetfHealth(h);
}

void CScriptGameObject::ActorSetPower(float p)
{
	CActor* actor = smart_cast<CActor*>(&object());
	if (!actor)
		return;
	actor->SetPower(p);
}

void CScriptGameObject::ActorSetSpeedScale(float speed_scale)
{
	CActor* actor = smart_cast<CActor*>(&object());
	if (!actor)
		return;
	actor->m_fSpeedScale = speed_scale;
}

void CScriptGameObject::ActorSetSprintBlock(bool block)
{
	CActor* actor = smart_cast<CActor*>(&object());
	if (!actor)
		return;
	actor->m_fSprintBlock = block;
}

void CScriptGameObject::ActorSetAccelBlock(bool block)
{
	CActor* actor = smart_cast<CActor*>(&object());
	if (!actor)
		return;
	actor->m_fAccelBlock = block;
}

bool CScriptGameObject::Category(LPCSTR cmpc, LPCSTR cmps, LPCSTR cmpd) const
{
	PIItem item							= smart_cast<PIItem>(&object());
	return								(item) ? item->Category(cmpc, cmps, cmpd) : false;
}

bool CScriptGameObject::InHands()
{
	PIItem item							= smart_cast<PIItem>(&object());
	return								(item) ? item->InHands() : false;
}

float CScriptGameObject::GetProtection(u8 type)
{
	CCustomOutfit*	outfit					= smart_cast<CCustomOutfit*>(&object());
	CHelmet*		helmet					= smart_cast<CHelmet*>(&object());
	if (!outfit && !helmet)
	{
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"CInventoryOwner : cannot access class member GetProtection!");
		return								0.f;
	}
	ALife::EHitType hit_type				= (ALife::EHitType)type;
	return									(outfit) ? outfit->GetHitTypeProtection(hit_type) : helmet->GetHitTypeProtection(hit_type);
}

float CScriptGameObject::GetArmorLevel()
{
	CCustomOutfit*	outfit		= smart_cast<CCustomOutfit*>(&object());
	CHelmet*		helmet		= smart_cast<CHelmet*>(&object());
	if (!outfit && !helmet)
		return					0.f;

	return (outfit) ? outfit->m_fArmorLevel : helmet->m_fArmorLevel;
}

float CScriptGameObject::GetWeightDump()
{
	CCustomOutfit* outfit				= object().scast<CCustomOutfit*>();
	CArtefact* art						= object().scast<CArtefact*>();
	if (!outfit && !art)
		return							0.f;

	return								(outfit) ? outfit->m_fWeightDump : art->getWeightDump();
}

float CScriptGameObject::GetRecuperationFactor()
{
	CCustomOutfit*	outfit		= smart_cast<CCustomOutfit*>(&object());
	CHelmet*		helmet		= smart_cast<CHelmet*>(&object());
	if (!outfit && !helmet)
		return					0.f;

	return (outfit) ? outfit->m_fRecuperationFactor : helmet->m_fRecuperationFactor;
}

float CScriptGameObject::GetDrainFactor()
{
	if (auto outfit = object().scast<CCustomOutfit*>())
		return							outfit->m_fDrainFactor;
	return								1.f;
}

float CScriptGameObject::GetPowerLoss()
{
	CCustomOutfit* outfit		= smart_cast<CCustomOutfit*>(&object());
	if (!outfit)
		return					0.f;

	return outfit->m_fPowerLoss;
}

float CScriptGameObject::GetInertion(bool full)
{
	if (auto item = smart_cast<PIItem>(&object()))
		return							item->GetControlInertionFactor(full);
	return								1.f;
}

float CScriptGameObject::GetTotalVolume() const
{
	CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
	if (!inventory_owner)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CInventoryOwner : cannot access class member GetTotalVolume!");
		return			(false);
	}
	return				(inventory_owner->inventory().TotalVolume());
}

float CScriptGameObject::GetInventoryCapacity()
{
	CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
	if (!inventory_owner)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CInventoryOwner : cannot access class member GetInventoryCapacity!");
		return			false;
	}
	return				inventory_owner->InventoryCapacity();
}

CSE_Abstract* CScriptGameObject::giveItem(LPCSTR section, float condition, bool straight)
{
	if (auto go = smart_cast<CGameObject*>(&object()))
		return							go->giveItem(section, condition, straight);
	return								nullptr;
}

CSE_Abstract* CScriptGameObject::giveItems(LPCSTR section, u16 count, float condition, bool straight)
{
	if (auto go = smart_cast<CGameObject*>(&object()))
		return							go->giveItems(section, count, condition).back();
	return								nullptr;
}

float CScriptGameObject::Volume() const
{
	CInventoryItem* iitem			= smart_cast<CInventoryItem*>(&object());
	return							iitem ? iitem->Volume() : false;
}

bool CScriptGameObject::IsActive() const
{
	CArtefact* artefact				= smart_cast<CArtefact*>(&object());
	return							(artefact) ? artefact->IsActive() : false;
}

void CScriptGameObject::Activate()
{
	CArtefact* artefact				= smart_cast<CArtefact*>(&object());
	if (artefact)
		artefact->Activate			();
}

void CScriptGameObject::Deactivate()
{
	CArtefact* artefact				= smart_cast<CArtefact*>(&object());
	if (artefact)
		artefact->Deactivate		();
}

void CScriptGameObject::AmmoChangeCount(u16 val)
{
	CWeaponAmmo* ammo				= smart_cast<CWeaponAmmo*>(&object());
	if (ammo)
		ammo->ChangeAmmoCount		(val);
}

float CScriptGameObject::GetDepletionRate() const
{
	MAmountable* aiitem				= object().getModule<MAmountable>();
	return							(aiitem) ? aiitem->GetDepletionRate() : false;
}

float CScriptGameObject::GetDepletionSpeed() const
{
	MAmountable* aiitem				= object().getModule<MAmountable>();
	return							(aiitem) ? aiitem->GetDepletionSpeed() : false;
}

void CScriptGameObject::SetDepletionSpeed(float val)
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->SetDepletionSpeed	(val);
}

float CScriptGameObject::GetCapacity() const
{
	if (auto cont = object().getModule<MContainer>())
		return							cont->GetCapacity();

	if (auto aiitem = object().getModule<MAmountable>())
		return							aiitem->getCapacity();

	return								0.f;
}

float CScriptGameObject::GetAmount() const
{
	PIItem iitem					= smart_cast<PIItem>(&object());
	return							(iitem) ? iitem->GetAmount() : false;
}

void CScriptGameObject::SetAmount(float val)
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->SetAmount			(val);
}

void CScriptGameObject::ChangeAmount(float val)
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->ChangeAmount		(val);
}

float CScriptGameObject::GetFill() const
{
	PIItem item						= smart_cast<PIItem>(&object());
	return							(item) ? item->GetFill() : 0.f;
}

void CScriptGameObject::SetFill(float val)
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->SetFill				(val);
}

void CScriptGameObject::ChangeFill(float val)
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->ChangeFill			(val);
}

void CScriptGameObject::Deplete()
{
	if (auto aiitem = object().getModule<MAmountable>())
		aiitem->Deplete				();
}

bool CScriptGameObject::Empty() const
{
	MAmountable* aiitem				= object().getModule<MAmountable>();
	return							(aiitem) ? aiitem->Empty() : false;
}

bool CScriptGameObject::Full() const
{
	MAmountable* aiitem				= object().getModule<MAmountable>();
	return							(aiitem) ? aiitem->Full() : false;
}

u32 CScriptGameObject::Amount() const
{
	MMagazine* mag					= object().getModule<MMagazine>();
	return							(mag) ? mag->Amount() : false;
}

u32 CScriptGameObject::Capacity() const
{
	MMagazine* mag					=object().getModule<MMagazine>();
	return							(mag) ? mag->Capacity() : false;
}

bool CScriptGameObject::Discharge(CScriptGameObject* obj)
{
	if (auto owner = object().scast<CInventoryOwner*>())
		if (auto mag = obj->object().getModule<MMagazine>())
			return					owner->discharge(mag);
	return							false;
}

bool CScriptGameObject::CanTake(CScriptGameObject* obj)
{
	if (auto ammo = obj->object().scast<CWeaponAmmo*>())
	{
		auto mag					= object().getModule<MMagazine>();
		if (!mag)
			if (auto wpn = object().scast<CWeaponMagazined*>())
				if (wpn->IsTriStateReload())
					mag				= wpn->getMagazine();
		if (mag)
			return					mag->canTake(ammo);
	}
	return							false;
}

void CScriptGameObject::Transfer(u16 id) const
{
	if (auto item = smart_cast<PIItem>(&object()))
		item->O.transfer			(id);
}

void CScriptGameObject::SetInvIcon(u8 idx)
{
	PIItem item						= smart_cast<PIItem>(&object());
	if (item)
		item->SetInvIconIndex		(idx);
}

u8 CScriptGameObject::GetInvIconIndex() const
{
	PIItem item						= smart_cast<PIItem>(&object());
	if (item)
		return						item->GetInvIconIndex();

	return							0;
}

float CScriptGameObject::Power() const
{
	CArtefact* artefact					= object().scast<CArtefact*>();
	return								(artefact) ? artefact->getPower() : 0.f;
}

float CScriptGameObject::Radiation() const
{
	CArtefact* artefact					= object().scast<CArtefact*>();
	return								(artefact) ? artefact->getRadiation() : 0.f;
}

float CScriptGameObject::Absorbation C$(int hit_type)
{
	CArtefact* artefact					= object().scast<CArtefact*>();
	return								(artefact) ? artefact->getAbsorbation(hit_type) : 0.f;
}

bool CScriptGameObject::Aiming C$()
{
	CActor* actor						= object().scast<CActor*>();
	return								(actor) ? actor->IsZoomAimingMode() : false;
}

LPCSTR CScriptGameObject::getInvName() const
{
	if (auto item = object().scast<CInventoryItem*>())
		return							item->getName();
	return								"";
}

LPCSTR CScriptGameObject::getInvNameShort() const
{
	if (auto item = object().scast<CInventoryItem*>())
		return							item->getNameShort();
	return								"";
}

LPCSTR CScriptGameObject::getActionTitle(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			return						*action->title;
	return								nullptr;
}

LPCSTR CScriptGameObject::getQueryFunctor(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			return						action->query_functor_str.c_str();
	return								nullptr;
}

LPCSTR CScriptGameObject::getActionFunctor(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			return						action->action_functor_str.c_str();
	return								nullptr;
}

LPCSTR CScriptGameObject::getUseFunctor(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			return						action->use_functor_str.c_str();
	return								nullptr;
}

float CScriptGameObject::getActionDuration(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			return						action->duration;
	return								0.f;
}

CScriptGameObject* CScriptGameObject::getActionItem(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			if (action->item_id != u16_max)
				return					Level().Objects.net_Find(action->item_id)->scast<CGameObject*>()->lua_game_object();
	return								nullptr;
}

void CScriptGameObject::resetActionItemID(int num) const
{
	if (auto usable = object().getModule<MUsable>())
		if (auto action = usable->getAction(num))
			action->item_id				= u16_max;
}

bool CScriptGameObject::isAttached() const
{
	if (auto addon = object().getModule<MAddon>())
		return							!!addon->getSlot();
	return								false;
}

bool CScriptGameObject::tryAttach(CScriptGameObject* obj) const
{
	if (auto ao = object().getModule<MAddonOwner>())
		if (auto addon = obj->object().getModule<MAddon>())
			if (!addon->O.scast<CWeaponAmmo*>())
				return					ao->tryAttach(addon, false);
	return								false;
}

bool CScriptGameObject::tryTransfer(CScriptGameObject* obj, bool attach) const
{
	if (auto wpn = object().scast<CWeaponMagazined*>())
		return wpn->tryTransfer			(obj->object().getModule<MAddon>(), attach);
	return								false;
}

void CScriptGameObject::attachFinish(CScriptGameObject* obj) const
{
	if (auto ao = object().getModule<MAddonOwner>())
		if (auto addon = obj->object().getModule<MAddon>())
			ao->finishAttaching			(addon);
}

void CScriptGameObject::detachFinish() const
{
	if (auto addon = object().getModule<MAddon>())
		addon->getSlot()->parent_ao->finishDetaching(addon);
}

void CScriptGameObject::shift(int val) const
{
	if (auto addon = object().getModule<MAddon>())
		addon->getSlot()->shiftAddon	(addon, val);
}

void CScriptGameObject::fold() const
{
	if (auto foldable = object().getModule<MFoldable>())
		foldable->setStatus				(true);
}

void CScriptGameObject::unfold() const
{
	if (auto foldable = object().getModule<MFoldable>())
		foldable->setStatus				(false);
}

bool CScriptGameObject::isFolded() const
{
	if (auto foldable = object().getModule<MFoldable>())
		return							foldable->getStatus();
}

void CScriptGameObject::loadChamber(CScriptGameObject* obj) const
{
	if (auto cartridge = smart_cast<CWeaponAmmo*>(&obj->object()))
		if (auto wpn = smart_cast<CWeaponMagazined*>(&object()))
			wpn->loadChamber			(cartridge);
}

void CScriptGameObject::unloadChamber(CScriptGameObject* obj) const
{
	if (auto addon = obj->object().getModule<MAddon>())
		if (auto wpn = object().scast<CWeaponMagazined*>())
			wpn->unloadChamber			(addon);
}

bool CScriptGameObject::tryChargeMagazine(CScriptGameObject* obj) const
{
	if (auto wpn = object().scast<CWeaponAutomaticShotgun*>())
		if (auto ammo = obj->object().scast<CWeaponAmmo*>())
			return						wpn->tryChargeMagazine(ammo);
	return								false;
}

void CScriptGameObject::chargeMagazine(CScriptGameObject* obj) const
{
	if (auto cartridge = obj->object().scast<CWeaponAmmo*>())
		if (auto mag = object().getModule<MMagazine>())
			mag->loadCartridge			(cartridge);
}

CScriptGameObject* CScriptGameObject::lookingAt() const
{
	if (auto actor = object().scast<CActor*>())
		if (auto obj = actor->ObjectWeLookingAt())
			return						obj->lua_game_object();
	return								nullptr;
}

void CScriptGameObject::setPosition(Fvector pos) const
{
	if (auto pi = object().scast<CPhysicItem*>())
	{
		pi->deactivate_physics_shell	();
		pi->XFORM().translate_over		(pos);
		pi->CPhysicsShellHolder::activate_physic_shell();
	}
}

void CScriptGameObject::reloadMagazine() const
{
	if (auto wpn = object().scast<CWeaponMagazined*>())
		wpn->Reload						();
}

int CScriptGameObject::getArtefactModuleMode() const
{
	if (auto am = object().getModule<MArtefactModule>())
		return							am->getMode();
	return								-1;
}

void CScriptGameObject::setArtefactModuleMode(int val) const
{
	if (auto am = object().getModule<MArtefactModule>())
		am->setMode						(val);
}

#define SPECIFIC_CAST(A,B)\
B* A ()\
{\
	B				*l_tpEntity = smart_cast<B*>(&object());\
	if (!l_tpEntity)\
		return (0);\
				else\
		return l_tpEntity;\
};\

SPECIFIC_CAST(CScriptGameObject::cast_GameObject, CScriptGameObject);
SPECIFIC_CAST(CScriptGameObject::cast_Car, CCar);
SPECIFIC_CAST(CScriptGameObject::cast_Heli, CHelicopter);
SPECIFIC_CAST(CScriptGameObject::cast_HolderCustom, CHolderCustom);
SPECIFIC_CAST(CScriptGameObject::cast_EntityAlive, CEntityAlive);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryItem, CInventoryItem);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryOwner, CInventoryOwner);
SPECIFIC_CAST(CScriptGameObject::cast_Actor, CActor);
SPECIFIC_CAST(CScriptGameObject::cast_Weapon, CWeapon);
CEatableItem* CScriptGameObject::cast_EatableItem()
{
	return object().scast<CEatableItem*>();
}
SPECIFIC_CAST(CScriptGameObject::cast_CustomOutfit, CCustomOutfit);
SPECIFIC_CAST(CScriptGameObject::cast_WeaponMagazined, CWeaponMagazined);
SPECIFIC_CAST(CScriptGameObject::cast_SpaceRestrictor, CSpaceRestrictor);
SPECIFIC_CAST(CScriptGameObject::cast_Stalker, CAI_Stalker);
SPECIFIC_CAST(CScriptGameObject::cast_CustomZone, CCustomZone);
SPECIFIC_CAST(CScriptGameObject::cast_Monster, CCustomMonster);
SPECIFIC_CAST(CScriptGameObject::cast_Explosive, CExplosive);
SPECIFIC_CAST(CScriptGameObject::cast_ScriptZone, CScriptZone);
//SPECIFIC_CAST(CScriptGameObject::cast_Projector, CProjector);
SPECIFIC_CAST(CScriptGameObject::cast_Trader, CAI_Trader);
CHudItem* CScriptGameObject::cast_HudItem()
{
	CInventoryItem* ii = object().cast_inventory_item();
	return ii ? smart_cast<CHudItem*>(ii) : (0);
}
SPECIFIC_CAST(CScriptGameObject::cast_Artefact, CArtefact);
SPECIFIC_CAST(CScriptGameObject::cast_Ammo, CWeaponAmmo);
//SPECIFIC_CAST(CScriptGameObject::cast_Missile, CMissile);
SPECIFIC_CAST(CScriptGameObject::cast_PhysicsShellHolder, CPhysicsShellHolder);
//SPECIFIC_CAST(CScriptGameObject::cast_Grenade, CGrenade);
CBottleItem* CScriptGameObject::cast_BottleItem()
{
	CInventoryItem* ii = object().cast_inventory_item();
	return ii ? smart_cast<CBottleItem*>(ii) : (0);
}
CTorch* CScriptGameObject::cast_Torch()
{

	CInventoryItem* ii = object().cast_inventory_item();
	return ii ? smart_cast<CTorch*>(ii) : (0);
}
SPECIFIC_CAST(CScriptGameObject::cast_WeaponMagazinedWGrenade, CWeaponMagazinedWGrenade);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryBox, CInventoryBox);
