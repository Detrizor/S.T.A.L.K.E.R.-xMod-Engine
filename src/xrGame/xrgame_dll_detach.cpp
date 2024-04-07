#include "pch_script.h"
#include "ai_space.h"
#include "object_factory.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "string_table.h"

#include "entity_alive.h"
#include "ui/UIInventoryUtilities.h"
#include "UI/UIXmlInit.h"
#include "UI/UItextureMaster.h"

#include "InfoPortion.h"
#include "PhraseDialog.h"
#include "GameTask.h"
#include "encyclopedia_article.h"

#include "character_info.h"
#include "specific_character.h"
#include "character_community.h"
#include "monster_community.h"
#include "character_rank.h"
#include "character_reputation.h"

#include "profiler.h"

#include "sound_collection_storage.h"
#include "relation_registry.h"

#include "ui\UIStatic.h"

typedef xr_vector<std::pair<shared_str,int> >	STORY_PAIRS;
extern STORY_PAIRS								story_ids;
extern STORY_PAIRS								spawn_story_ids;

extern void show_smart_cast_stats					();
extern void clear_smart_cast_stats					();
extern void release_smart_cast_stats				();
extern void dump_list_wnd							();
extern void dump_list_lines							();
extern void dump_list_sublines						();
extern void clean_wnd_rects							();
extern void dump_list_xmls							();
extern void CreateUIGeom							();
extern void DestroyUIGeom							();
extern void InitHudSoundSettings					();

#include "../xrEngine/IGame_Persistent.h"
void init_game_globals()
{
	CreateUIGeom									();
	InitHudSoundSettings							();
	if(!g_dedicated_server)
	{
//		CInfoPortion::InitInternal					();
//.		CEncyclopediaArticle::InitInternal			();
		CPhraseDialog::InitInternal					();
		InventoryUtilities::CreateShaders			();
	};
	CCharacterInfo::InitInternal					();
	CSpecificCharacter::InitInternal				();
	CHARACTER_COMMUNITY::InitInternal				();
	CHARACTER_RANK::InitInternal					();
	CHARACTER_REPUTATION::InitInternal				();
	MONSTER_COMMUNITY::InitInternal					();
}

extern CUIXml*		g_uiSpotXml;

void clean_game_globals()
{
	// destroy ai space
	xr_delete										(g_ai_space);
	// destroy object factory
	xr_delete										(g_object_factory);
	// destroy monster squad global var
	xr_delete										(g_monster_squad);

	story_ids.clear									();
	spawn_story_ids.clear							();

	if(!g_dedicated_server)
	{
//.		CInfoPortion::DeleteSharedData					();
//.		CInfoPortion::DeleteIdToIndexData				();

//.		CEncyclopediaArticle::DeleteSharedData			();
//.		CEncyclopediaArticle::DeleteIdToIndexData		();

		CPhraseDialog::DeleteSharedData					();
		CPhraseDialog::DeleteIdToIndexData				();
		
		InventoryUtilities::DestroyShaders				();
	}
	CCharacterInfo::DeleteSharedData				();
	CCharacterInfo::DeleteIdToIndexData				();
	
	CSpecificCharacter::DeleteSharedData			();
	CSpecificCharacter::DeleteIdToIndexData			();

	CHARACTER_COMMUNITY::DeleteIdToIndexData		();
	CHARACTER_RANK::DeleteIdToIndexData				();
	CHARACTER_REPUTATION::DeleteIdToIndexData		();
	MONSTER_COMMUNITY::DeleteIdToIndexData			();


	//static shader for blood
	CEntityAlive::UnloadBloodyWallmarks				();
	CEntityAlive::UnloadFireParticles				();
	//очищение памяти таблицы строк
	CStringTable::Destroy							();
	// Очищение таблицы цветов
	CUIXmlInit::DeleteColorDefs						();
	// Очищение таблицы идентификаторов рангов и отношений сталкеров
	InventoryUtilities::ClearCharacterInfoStrings	();

	xr_delete										(g_sound_collection_storage);
	
#ifdef DEBUG
	xr_delete										(g_profiler);
	release_smart_cast_stats						();
#endif

	RELATION_REGISTRY::clear_relation_registry		();

	dump_list_wnd									();
	dump_list_lines									();
	dump_list_sublines								();
	clean_wnd_rects									();
	xr_delete										(g_uiSpotXml);
	dump_list_xmls									();
	DestroyUIGeom									();
	CUITextureMaster::FreeTexInfo					();
}

#include "scope.h"
#include "EntityCondition.h"
#include "Level_Bullet_Manager.h"
#include "weapon_hud.h"
#include "items_library.h"

extern ENGINE_API float		psAIM_FOV;
float						aim_fov_tan;
float g_fov					= 75.f;

HitImmunity::HitTypeSVec CEntityCondition::HitTypeHeadPart;
HitImmunity::HitTypeSVec CEntityCondition::HitTypeGlobalScale;

float CEntityCondition::m_fMeleeOnPierceDamageMultiplier;
float CEntityCondition::m_fMeleeOnPierceArmorDamageFactor;

SPowerDependency CEntityCondition::ArmorDamageResistance;
SPowerDependency CEntityCondition::StrikeDamageThreshold;
SPowerDependency CEntityCondition::StrikeDamageResistance;
SPowerDependency CEntityCondition::ExplDamageResistance;

SPowerDependency CEntityCondition::AnomalyDamageThreshold;
SPowerDependency CEntityCondition::AnomalyDamageResistance;
SPowerDependency CEntityCondition::ProtectionDamageResistance;

SPowerDependency CWeaponHud::HandlingToRotationTime;

float CFireDispertionController::crosshair_inertion;


float CScope::s_magnification_eye_relief_shrink;
float CScope::s_lense_circle_scale_offset_power;
float CScope::s_lense_circle_position_derivation_factor;

extern CUIStatic*			pUILenseCircle;
extern CUIStatic*			pUILenseBlackFill;
extern CUIStatic*			pUILenseGlass;
extern void createStatic	(CUIStatic*& dest, LPCSTR texture, float mult = 1.f, EAlignment al = aCenter);

void loadStaticVariables()
{
	CEntityCondition::HitTypeHeadPart.resize						(ALife::eHitTypeMax);
	for (int i = 0; i < ALife::eHitTypeMax; i++)
		CEntityCondition::HitTypeHeadPart[i] = 0.f;
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeBurn]			= pSettings->r_float("hit_type_head_part", "burn");
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeShock]			= pSettings->r_float("hit_type_head_part", "shock");
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeRadiation]		= pSettings->r_float("hit_type_head_part", "radiation");
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeChemicalBurn]	= pSettings->r_float("hit_type_head_part", "chemical_burn");
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeTelepatic]		= pSettings->r_float("hit_type_head_part", "telepatic");
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeLightBurn]		= CEntityCondition::HitTypeHeadPart[ALife::eHitTypeBurn];
	CEntityCondition::HitTypeHeadPart[ALife::eHitTypeExplosion]		= pSettings->r_float("hit_type_head_part", "explosion");
	
	CEntityCondition::HitTypeGlobalScale.resize							(ALife::eHitTypeMax);
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeBurn]			= pSettings->r_float("hit_type_global_scale", "burn");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeShock]			= pSettings->r_float("hit_type_global_scale", "shock");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeChemicalBurn]	= pSettings->r_float("hit_type_global_scale", "chemical_burn");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeRadiation]		= pSettings->r_float("hit_type_global_scale", "radiation");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeTelepatic]		= pSettings->r_float("hit_type_global_scale", "telepatic");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeWound]			= pSettings->r_float("hit_type_global_scale", "wound");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeFireWound]		= pSettings->r_float("hit_type_global_scale", "fire_wound");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeStrike]			= pSettings->r_float("hit_type_global_scale", "strike");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeExplosion]		= pSettings->r_float("hit_type_global_scale", "explosion");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeWound_2]		= pSettings->r_float("hit_type_global_scale", "wound_2");
	CEntityCondition::HitTypeGlobalScale[ALife::eHitTypeLightBurn]		= pSettings->r_float("hit_type_global_scale", "light_burn");
	
	CEntityCondition::m_fMeleeOnPierceDamageMultiplier		= pSettings->r_float("damage_manager", "melee_on_pierce_damage_multiplier");
	CEntityCondition::m_fMeleeOnPierceArmorDamageFactor		= pSettings->r_float("damage_manager", "melee_on_pierce_armor_damage_factor");

	CEntityCondition::StrikeDamageThreshold.Load		("damage_manager", "strike_damage_threshold");
	CEntityCondition::StrikeDamageResistance.Load		("damage_manager", "strike_damage_resistance");
	CEntityCondition::ExplDamageResistance.Load			("damage_manager", "expl_damage_resistance");
	CEntityCondition::ArmorDamageResistance.Load		("damage_manager", "armor_damage_resistance");
	
	CEntityCondition::AnomalyDamageThreshold.Load		("damage_manager", "anomaly_damage_threshold");
	CEntityCondition::AnomalyDamageResistance.Load		("damage_manager", "anomaly_damage_resistance");
	CEntityCondition::ProtectionDamageResistance.Load	("damage_manager", "protection_damage_resistance");

	CFireDispertionController::crosshair_inertion		= pSettings->r_float("weapon_manager", "crosshair_inertion");
	CWeaponHud::HandlingToRotationTime.Load				("weapon_manager", "handling_to_rotation_time");

	CScope::s_magnification_eye_relief_shrink			= pSettings->r_float("weapon_manager", "magnification_eye_relief_shrink");
	CScope::s_lense_circle_scale_offset_power			= pSettings->r_float("weapon_manager", "lense_circle_scale_offset_power");
	CScope::s_lense_circle_position_derivation_factor	= pSettings->r_float("weapon_manager", "lense_circle_position_derivation_factor");

	psAIM_FOV		= pSettings->r_float("weapon_manager", "aim_fov");
	aim_fov_tan		= tanf(psAIM_FOV * (0.5f * PI / 180.f));

	g_items_library = xr_new<CItemsLibrary>();

	createStatic(pUILenseCircle, "wpn\\lense\\circle", 4.f);
	createStatic(pUILenseBlackFill, "wpn\\lense\\black_fill", .125f, aLeftTop);
	createStatic(pUILenseGlass, "wpn\\lense\\glass");
}

void cleanStaticVariables()
{
	xr_delete(g_items_library);
	xr_delete(pUILenseCircle);
	xr_delete(pUILenseBlackFill);
	xr_delete(pUILenseGlass);
}
