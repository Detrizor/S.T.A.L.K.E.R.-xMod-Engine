
#include "pch_script.h"
#include "UIInventoryUtilities.h"
#include "../WeaponAmmo.h"
#include "../UIStaticItem.h"
#include "UIStatic.h"
#include "../eatable_item.h"
#include "../Level.h"
#include "../date_time.h"
#include "../string_table.h"
#include "../Inventory.h"
#include "../InventoryOwner.h"
#include "../InventoryBox.h"
#include "../item_container.h"

#include "../InfoPortion.h"
#include "game_base_space.h"
#include "../actor.h"

#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"

#include "../Include/xrRender/UIShader.h"

#include "UICellItem.h"

#define BUY_MENU_TEXTURE "ui\\ui_mp_buy_menu"
#define CHAR_ICONS		 "ui\\ui_icons_npc"
#define MAP_ICONS		 "ui\\ui_icons_map"
#define MP_CHAR_ICONS	 "ui\\ui_models_multiplayer"

const float inv_grid_size			= pSettings->r_float("miscellaneous", "inv_grid_size");
const float inv_icon_spacing		= pSettings->r_float("miscellaneous", "inv_icon_spacing");

const LPCSTR relationsLtxSection	= "game_relations";
const LPCSTR ratingField			= "rating_names";
const LPCSTR reputationgField		= "reputation_names";
const LPCSTR goodwillField			= "goodwill_names";

const LPCSTR st_months[12]= // StringTable for GetDateAsString()
{
	"month_january",
	"month_february",
	"month_march",
	"month_april",
	"month_may",
	"month_june",
	"month_july",
	"month_august",
	"month_september",
	"month_october",
	"month_november",
	"month_december"
};

associative_vector<shared_str, ui_shader*> g_EquipmentIconsShader;

ui_shader	*g_BuyMenuShader			= NULL;
ui_shader	*g_MPCharIconsShader		= NULL;
ui_shader	*g_OutfitUpgradeIconsShader	= NULL;
ui_shader	*g_WeaponUpgradeIconsShader	= NULL;
ui_shader	*g_tmpWMShader				= NULL;
static CUIStatic*	GetUIStatic				();

typedef				std::pair<CHARACTER_RANK_VALUE, shared_str>	CharInfoStringID;
DEF_MAP				(CharInfoStrings, CHARACTER_RANK_VALUE, shared_str);

CharInfoStrings		*charInfoReputationStrings	= NULL;
CharInfoStrings		*charInfoRankStrings		= NULL;
CharInfoStrings		*charInfoGoodwillStrings	= NULL;

void InventoryUtilities::CreateShaders()
{
	g_tmpWMShader = xr_new<ui_shader>();
	(*g_tmpWMShader)->create("effects\\wallmark",  "wm\\wm_grenade");
	//g_tmpWMShader.create("effects\\wallmark",  "wm\\wm_grenade");

	g_EquipmentIconsShader.clear();
}

void InventoryUtilities::DestroyShaders()
{
	xr_delete(g_BuyMenuShader);
	g_BuyMenuShader = 0;

	xr_delete(g_MPCharIconsShader);
	g_MPCharIconsShader = 0;

	xr_delete(g_OutfitUpgradeIconsShader);
	g_OutfitUpgradeIconsShader = 0;

	xr_delete(g_WeaponUpgradeIconsShader);
	g_WeaponUpgradeIconsShader = 0;

	xr_delete(g_tmpWMShader);
	g_tmpWMShader = 0;

	for (associative_vector<shared_str, ui_shader*>::iterator it = g_EquipmentIconsShader.begin(), it_e = g_EquipmentIconsShader.end(); it != it_e; it++)
		xr_delete(it->second);
	g_EquipmentIconsShader.clear();
}

bool InventoryUtilities::GreaterRoomInRuck(PIItem item1, PIItem item2)
{
	auto& r1							= item1->getIcon()->GetGridSize();
	auto& r2							= item2->getIcon()->GetGridSize();

	if (r1.x > r2.x)
		return							true;
	
	if (r1.x == r2.x)
	{
		if (r1.y > r2.y)
			return						true;
		
		if (r1.y == r2.y)
		{
			if (item1->object().cNameSect() == item2->object().cNameSect())
				return					(item1->object().ID() > item2->object().ID());
			else
				return					(item1->object().cNameSect() > item2->object().cNameSect());

		}

		return							false;
	}

	return								false;
}

const ui_shader& InventoryUtilities::GetBuyMenuShader()
{	
	if(!g_BuyMenuShader)
	{
		g_BuyMenuShader = xr_new<ui_shader>();
		(*g_BuyMenuShader)->create("hud\\default", BUY_MENU_TEXTURE);
	}

	return *g_BuyMenuShader;
}

const ui_shader& InventoryUtilities::GetEquipmentIconsShader(const shared_str& section)
{
	shared_str tex = pSettings->line_exist(section, "icon_texture") ? pSettings->r_string(section, "icon_texture") : pSettings->r_string(section, "category");
	associative_vector<shared_str, ui_shader*>::iterator it = g_EquipmentIconsShader.find(tex);
	if (it  == g_EquipmentIconsShader.end())
	{
		ui_shader* tmp = xr_new<ui_shader>();
		(*tmp)->create("hud\\default", shared_str().printf("ui\\icon\\%s", *tex).c_str());
		g_EquipmentIconsShader.insert(std::make_pair(tex, tmp));
		it = g_EquipmentIconsShader.find(tex);
	}

	return *it->second;
}

const ui_shader&	InventoryUtilities::GetMPCharIconsShader()
{
	if(!g_MPCharIconsShader)
	{
		g_MPCharIconsShader = xr_new<ui_shader>();
		(*g_MPCharIconsShader)->create("hud\\default",  MP_CHAR_ICONS);
	}

	return *g_MPCharIconsShader;
}

const ui_shader& InventoryUtilities::GetOutfitUpgradeIconsShader()
{	
	if(!g_OutfitUpgradeIconsShader)
	{
		g_OutfitUpgradeIconsShader = xr_new<ui_shader>();
		(*g_OutfitUpgradeIconsShader)->create("hud\\default", "ui\\ui_actor_armor");
	}

	return *g_OutfitUpgradeIconsShader;
}

const ui_shader& InventoryUtilities::GetWeaponUpgradeIconsShader()
{	
	if(!g_WeaponUpgradeIconsShader)
	{
		g_WeaponUpgradeIconsShader = xr_new<ui_shader>();
		(*g_WeaponUpgradeIconsShader)->create("hud\\default", "ui\\ui_actor_weapons");
	}

	return *g_WeaponUpgradeIconsShader;
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetGameDateAsString(EDatePrecision datePrec, char dateSeparator)
{
	return GetDateAsString(Level().GetGameTime(), datePrec, dateSeparator);
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetGameTimeAsString(ETimePrecision timePrec, char timeSeparator)
{
	return GetTimeAsString(Level().GetGameTime(), timePrec, timeSeparator);
}

const shared_str InventoryUtilities::GetTimeAndDateAsString(ALife::_TIME_ID time)
{
	string256 buf;
	LPCSTR time_str = GetTimeAsString( time, etpTimeToMinutes ).c_str();
	LPCSTR date_str = GetDateAsString( time, edpDateToDay ).c_str();
	strconcat( sizeof(buf), buf, time_str, ", ", date_str );
	return buf;
}

const shared_str InventoryUtilities::Get_GameTimeAndDate_AsString()
{
	return GetTimeAndDateAsString( Level().GetGameTime() );
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetTimeAsString(ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator, bool full_mode )
{
	string32 bufTime;

	ZeroMemory(bufTime, sizeof(bufTime));

	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

	split_time(time, year, month, day, hours, mins, secs, milisecs);

	// Time
	switch (timePrec)
	{
	case etpTimeToHours:
		xr_sprintf(bufTime, "%02i", hours);
		break;
	case etpTimeToMinutes:
		if ( full_mode || hours > 0 ) {
			xr_sprintf(bufTime, "%02i%c%02i", hours, timeSeparator, mins);
			break;
		}
		xr_sprintf(bufTime, "0%c%02i", timeSeparator, mins);
		break;
	case etpTimeToSeconds:
		if ( full_mode || hours > 0 ) {
			xr_sprintf(bufTime, "%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs);
			break;
		}
		if ( mins > 0 ) {
			xr_sprintf(bufTime, "%02i%c%02i", mins, timeSeparator, secs);
			break;
		}
		xr_sprintf(bufTime, "0%c%02i", timeSeparator, secs);
		break;
	case etpTimeToMilisecs:
		xr_sprintf(bufTime, "%02i%c%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs, timeSeparator, milisecs);
		break;
	case etpTimeToSecondsAndDay:
		{
			int total_day = (int)( time/(1000*60*60*24) );
			xr_sprintf(bufTime, sizeof(bufTime), "%dd %02i%c%02i%c%02i", total_day, hours, timeSeparator, mins, timeSeparator, secs);
			break;
		}
	default:
		R_ASSERT(!"Unknown type of date precision");
	}

	return bufTime;
}

const shared_str InventoryUtilities::GetDateAsString(ALife::_TIME_ID date, EDatePrecision datePrec, char dateSeparator)
{
	string64 bufDate;

	ZeroMemory(bufDate, sizeof(bufDate));

	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

	split_time(date, year, month, day, hours, mins, secs, milisecs);
	VERIFY( 1 <= month && month <= 12 );
	LPCSTR month_str = CStringTable().translate( st_months[month-1] ).c_str();

	// Date
	switch (datePrec)
	{
	case edpDateToYear:
		xr_sprintf(bufDate, "%04i", year);
		break;
	case edpDateToMonth:
		xr_sprintf(bufDate, "%s%c% 04i", month_str, dateSeparator, year);
		break;
	case edpDateToDay:
		xr_sprintf(bufDate, "%s %d%c %04i", month_str, day, dateSeparator, year);
		break;
	default:
		R_ASSERT(!"Unknown type of date precision");
	}

	return bufDate;
}

LPCSTR InventoryUtilities::GetTimePeriodAsString(LPSTR _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to)
{
	u32 year1,month1,day1,hours1,mins1,secs1,milisecs1;
	u32 year2,month2,day2,hours2,mins2,secs2,milisecs2;
	
	split_time(_from, year1, month1, day1, hours1, mins1, secs1, milisecs1);
	split_time(_to, year2, month2, day2, hours2, mins2, secs2, milisecs2);
	
	int cnt		= 0;
	_buff[0]	= 0;

	u8 yrdiff = ((year2 - year1) * 12);

	if (month1 != month2 || yrdiff > 0)
		cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", month2 + (yrdiff - month1), *CStringTable().translate("ui_st_months"));

	if(!cnt && day1!=day2)
		cnt = xr_sprintf(_buff+cnt,buff_sz-cnt,"%d %s",day2-day1, *CStringTable().translate("ui_st_days"));

	if(!cnt && hours1!=hours2)
		cnt = xr_sprintf(_buff+cnt,buff_sz-cnt,"%d %s",hours2-hours1, *CStringTable().translate("ui_st_hours"));

	if(!cnt && mins1!=mins2)
		cnt = xr_sprintf(_buff+cnt,buff_sz-cnt,"%d %s",mins2-mins1, *CStringTable().translate("ui_st_mins"));

	if(!cnt && secs1!=secs2)
		cnt = xr_sprintf(_buff+cnt,buff_sz-cnt,"%d %s",secs2-secs1, *CStringTable().translate("ui_st_secs"));

	return _buff;
}

//////////////////////////////////////////////////////////////////////////

void InventoryUtilities::UpdateLabelsValues(CUITextWnd* pWeight, CUITextWnd* pVolume, CInventoryOwner* pInventoryOwner, MContainer* cont)
{
	float								total_weight, total_volume, max_volume;
	if (pInventoryOwner)
	{
		total_weight					= pInventoryOwner->inventory().CalcTotalWeight();
		total_volume					= pInventoryOwner->inventory().CalcTotalVolume();
		max_volume						= 666.f;
	}
	else
	{
		total_weight					= cont->GetWeight();
		total_volume					= cont->GetVolume();
		max_volume						= cont->GetCapacity();
	}
	
	bool actors							= !!smart_cast<CActor*>(pInventoryOwner);
	u32									color_weight, color_volume;
	if (pInventoryOwner && pInventoryOwner->is_alive())
	{
		float d_weight					= total_weight / 100.f;
		clamp<float>					(d_weight, 0.f, 1.f);
		u32 red							= u32(255.f * d_weight) * 2;
		clamp<u32>						(red, 0, 255);
		u32 green						= u32(255.f * (1 - d_weight)) * 2;
		clamp<u32>						(green, 0, 255);
		color_weight					= color_argb(255, red, green, 0);
		color_volume					= ((actors || cont) && (total_volume > max_volume)) ? color_argb(255, 255, 0, 0) : color_argb(255, 255, 255, 255);
	}
	else
		color_weight = color_volume		= color_argb(255, 255, 255, 255);

	string128							buf;

	LPCSTR kg_str						= CStringTable().translate( "st_kg" ).c_str();
	xr_sprintf							(buf, "%.2f %s", total_weight, kg_str);
	pWeight->SetText					(buf);
	pWeight->SetTextColor				(color_weight);
	pWeight->AdjustWidthToText			();

	LPCSTR li_str						= CStringTable().translate("st_li").c_str();
	if (actors || cont)
		xr_sprintf						(buf, "%.2f/%.2f %s", total_volume, max_volume, li_str);
	else
		xr_sprintf						(buf, "%.2f %s", total_volume, li_str);
	pVolume->SetText					(buf);
	pVolume->SetTextColor				(color_volume);
	pVolume->AdjustWidthToText			();
}

void InventoryUtilities::AlighLabels(CUIStatic* pWeightInfo, CUITextWnd* pWeight, CUIStatic* pVolumeInfo, CUITextWnd* pVolume)
{
	Fvector2 pos						= pWeightInfo->GetWndPos();
	pos.x								+= pWeightInfo->GetWndSize().x + 5.f;
	pWeight->SetWndPos					(pos);

	pos.x								+= pWeight->GetWndSize().x + 5.f;
	pVolumeInfo->SetWndPos				(pos);

	pos.x								+= pVolumeInfo->GetWndSize().x + 5.f;
	pVolume->SetWndPos					(pos);
}

//////////////////////////////////////////////////////////////////////////

void LoadStrings(CharInfoStrings *container, LPCSTR section, LPCSTR field)
{
	R_ASSERT(container);

	LPCSTR				cfgRecord	= pSettings->r_string(section, field);
	u32					count		= _GetItemCount(cfgRecord);
	R_ASSERT3			(count%2, "there're must be an odd number of elements", field);
	string64			singleThreshold;
	ZeroMemory			(singleThreshold, sizeof(singleThreshold));
	int					upBoundThreshold	= 0;
	CharInfoStringID	id;

	for (u32 k = 0; k < count; k += 2)
	{
		_GetItem(cfgRecord, k, singleThreshold);
		id.second = singleThreshold;

		_GetItem(cfgRecord, k + 1, singleThreshold);
		if(k+1!=count)
			sscanf(singleThreshold, "%i", &upBoundThreshold);
		else
			upBoundThreshold	+= 1;

		id.first = upBoundThreshold;

		container->insert(id);
	}
}

//////////////////////////////////////////////////////////////////////////

void InitCharacterInfoStrings()
{
	if (charInfoReputationStrings && charInfoRankStrings) return;

	if (!charInfoReputationStrings)
	{
		// Create string->Id DB
		charInfoReputationStrings	= xr_new<CharInfoStrings>();
		// Reputation
		LoadStrings(charInfoReputationStrings, relationsLtxSection, reputationgField);
	}

	if (!charInfoRankStrings)
	{
		// Create string->Id DB
		charInfoRankStrings			= xr_new<CharInfoStrings>();
		// Ranks
		LoadStrings(charInfoRankStrings, relationsLtxSection, ratingField);
	}

	if (!charInfoGoodwillStrings)
	{
		// Create string->Id DB
		charInfoGoodwillStrings			= xr_new<CharInfoStrings>();
		// Goodwills
		LoadStrings(charInfoGoodwillStrings, relationsLtxSection, goodwillField);
	}

}

//////////////////////////////////////////////////////////////////////////

void InventoryUtilities::ClearCharacterInfoStrings()
{
	xr_delete(charInfoReputationStrings);
	xr_delete(charInfoRankStrings);
	xr_delete(charInfoGoodwillStrings);
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetRankAsText(CHARACTER_RANK_VALUE rankID)
{
	InitCharacterInfoStrings();
	CharInfoStrings::const_iterator cit = charInfoRankStrings->upper_bound(rankID);
	if(charInfoRankStrings->end() == cit)
		return charInfoRankStrings->rbegin()->second.c_str();
	return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetReputationAsText(CHARACTER_REPUTATION_VALUE rankID)
{
	InitCharacterInfoStrings();

	CharInfoStrings::const_iterator cit = charInfoReputationStrings->upper_bound(rankID);
	if(charInfoReputationStrings->end() == cit)
		return charInfoReputationStrings->rbegin()->second.c_str();

	return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetGoodwillAsText(CHARACTER_GOODWILL goodwill)
{
	InitCharacterInfoStrings();

	CharInfoStrings::const_iterator cit = charInfoGoodwillStrings->upper_bound(goodwill);
	if(charInfoGoodwillStrings->end() == cit)
		return charInfoGoodwillStrings->rbegin()->second.c_str();

	return cit->second.c_str();
}


//////////////////////////////////////////////////////////////////////////
// специальная функция для передачи info_portions при нажатии кнопок UI 
// (для tutorial)
void InventoryUtilities::SendInfoToActor(LPCSTR info_id)
{
	CActor* actor = smart_cast<CActor*>(Level().CurrentEntity());
	if(actor)
	{
		actor->TransferInfo(info_id, true);
	}
}

void InventoryUtilities::SendInfoToLuaScripts(shared_str info)
{
	if ( info == shared_str("ui_talk_show") )
	{
		int mode = 10; // now Menu is Talk Dialog (show)
		luabind::functor<void>	funct;
		R_ASSERT( ai().script_engine().functor( "pda.actor_menu_mode", funct ) );
		funct( mode );
	}
	if ( info == shared_str("ui_talk_hide") )
	{
		int mode = 11; // Talk Dialog hide
		luabind::functor<void>	funct;
		R_ASSERT( ai().script_engine().functor( "pda.actor_menu_mode", funct ) );
		funct( mode );
	}
}

u32 InventoryUtilities::GetGoodwillColor(CHARACTER_GOODWILL gw)
{
	u32 res = 0xffc0c0c0;
	if(gw==NEUTRAL_GOODWILL){
		res = 0xfffce80b; //0xffc0c0c0;
	}else
	if(gw>1000){
		res = 0xff00ff00;
	}else
	if(gw<-1000){
		res = 0xffff0000;
	}
	return res;
}

u32 InventoryUtilities::GetReputationColor(CHARACTER_REPUTATION_VALUE rv)
{
	u32 res = 0xffc0c0c0;
	if(rv==NEUTAL_REPUTATION){
		res = 0xffc0c0c0;
	}else
	if(rv>50){
		res = 0xff00ff00;
	}else
	if(rv<-50){
		res = 0xffff0000;
	}
	return res;
}

u32	InventoryUtilities::GetRelationColor(ALife::ERelationType relation)
{
	switch(relation) {
	case ALife::eRelationTypeFriend:
		return 0xff00ff00;
		break;
	case ALife::eRelationTypeNeutral:
		return 0xfffce80b; //0xffc0c0c0;
		break;
	case ALife::eRelationTypeEnemy:
		return  0xffff0000;
		break;
	default:
		NODEFAULT;
	}
#ifdef DEBUG
	return 0xffffffff;
#endif
}

Ivector2 InventoryUtilities::CalculateIconSize(Frect CR$ icon_rect, float icon_scale)
{
	return								{
		iCeil((icon_scale * icon_rect.width() + 2.f * inv_icon_spacing) / inv_grid_size),
		iCeil((icon_scale * icon_rect.height() + 2.f * inv_icon_spacing) / inv_grid_size)
	};
}

Ivector2 InventoryUtilities::CalculateIconSize(Frect CR$ icon_rect, float icon_scale, Frect& margin)
{
	Ivector2 res						= CalculateIconSize(icon_rect, icon_scale);

	float cell_width					= static_cast<float>(res.x) * inv_grid_size;
	float cell_height					= static_cast<float>(res.y) * inv_grid_size;
	margin.x1							= margin.x2 = .5f * (cell_width - icon_scale * icon_rect.width()) / cell_width;
	margin.y1							= margin.y2 = .5f * (cell_height - icon_scale * icon_rect.height()) / cell_height;

	return								res;
}

Ivector2 InventoryUtilities::CalculateIconSize(Frect CR$ icon_rect, Frect& margin, Frect CR$ addons_rect)
{
	Ivector2 res						= CalculateIconSize(addons_rect, 1.f);

	float cell_width					= static_cast<float>(res.x) * inv_grid_size;
	float cell_height					= static_cast<float>(res.y) * inv_grid_size;
	margin.left							= (icon_rect.left - addons_rect.left + .5f * (cell_width - addons_rect.width())) / cell_width;
	margin.top							= (icon_rect.top - addons_rect.top + .5f * (cell_height - addons_rect.height())) / cell_height;
	margin.right						= (addons_rect.right - icon_rect.right + .5f * (cell_width - addons_rect.width())) / cell_width;
	margin.bottom						= (addons_rect.bottom - icon_rect.bottom + .5f * (cell_height - addons_rect.height())) / cell_height;

	return								res;
}
