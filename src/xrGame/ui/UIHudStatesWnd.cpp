#include "stdafx.h"
#include "UIHudStatesWnd.h"

#include "../Actor.h"
#include "../ActorCondition.h"
#include "../EntityCondition.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../inventory.h"
#include "../RadioactiveZone.h"

#include "UIStatic.h"
#include "UIProgressBar.h"
#include "UIProgressShape.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "ui_arrow.h"
#include "UIInventoryUtilities.h"
#include "CustomDetector.h"
#include "../ai/monsters/basemonster/base_monster.h"
#include "../PDA.h"
#include "WeaponMagazinedWGrenade.h"

CUIHudStatesWnd::CUIHudStatesWnd()
:m_b_force_update(true),
	m_timer_1sec(0),
	m_last_health(0.0f),
	m_radia_self(0.0f),
	m_radia_hit(0.0f)
{

	for ( int i = 0; i < ALife::infl_max_count; ++i )
	{
		m_zone_cur_power[i] = 0.0f;
//--		m_zone_max_power[i] = 1.0f;
		m_zone_feel_radius[i] = 1.0f;
	}
	m_zone_hit_type[ALife::infl_rad ] = ALife::eHitTypeRadiation;
	m_zone_hit_type[ALife::infl_fire] = ALife::eHitTypeBurn;
	m_zone_hit_type[ALife::infl_acid] = ALife::eHitTypeChemicalBurn;
	m_zone_hit_type[ALife::infl_psi ] = ALife::eHitTypeTelepatic;
	m_zone_hit_type[ALife::infl_electra] = ALife::eHitTypeShock;

	m_zone_feel_radius_max = 0.0f;
	
	m_health_blink = pSettings->r_float( "actor_condition", "hud_health_blink" );
	clamp( m_health_blink, 0.0f, 1.0f );

	m_fake_indicators_update = false;
//-	Load_section();
}

CUIHudStatesWnd::~CUIHudStatesWnd()
{
}

void CUIHudStatesWnd::reset_ui()
{
	if ( g_pGameLevel )
	{
		Level().hud_zones_list->clear();
	}
}

ALife::EInfluenceType CUIHudStatesWnd::get_indik_type( ALife::EHitType hit_type )
{
	ALife::EInfluenceType iz_type = ALife::infl_max_count;
	switch( hit_type )
	{
	case ALife::eHitTypeRadiation:		iz_type = ALife::infl_rad;		break;
	case ALife::eHitTypeLightBurn:
	case ALife::eHitTypeBurn:			iz_type = ALife::infl_fire;		break;
	case ALife::eHitTypeChemicalBurn:	iz_type = ALife::infl_acid;		break;
	case ALife::eHitTypeTelepatic:		iz_type = ALife::infl_psi;		break;
	case ALife::eHitTypeShock:			iz_type = ALife::infl_electra;	break;// it hasnt CStatic

	case ALife::eHitTypeStrike:
	case ALife::eHitTypeWound:
	case ALife::eHitTypeExplosion:
	case ALife::eHitTypeFireWound:
	case ALife::eHitTypeWound_2:
//	case ALife::eHitTypePhysicStrike:
		return ALife::infl_max_count;
	default:
		NODEFAULT;
	}
	return iz_type;
}

void CUIHudStatesWnd::InitFromXml( CUIXml& xml, LPCSTR path )
{
	CUIXmlInit::InitWindow( xml, path, 0, this );
	XML_NODE* stored_root = xml.GetLocalRoot();
	
	XML_NODE* new_root = xml.NavigateToNode( path, 0 );
	xml.SetLocalRoot( new_root );

	m_ui_health_bar   = UIHelper::CreateProgressBar( xml, "progress_bar_health", this );
	m_ui_stamina_bar  = UIHelper::CreateProgressBar( xml, "progress_bar_stamina", this );

	m_indik[ALife::infl_rad]  = UIHelper::CreateStatic( xml, "indik_rad", this );
	m_indik[ALife::infl_fire] = UIHelper::CreateStatic( xml, "indik_fire", this );
	m_indik[ALife::infl_acid] = UIHelper::CreateStatic( xml, "indik_acid", this );
	m_indik[ALife::infl_psi]  = UIHelper::CreateStatic( xml, "indik_psi", this );
	
	m_back								= UIHelper::CreateStatic( xml, "back", this );
	m_zeroing							= UIHelper::CreateTextWnd( xml, "static_zeroing", m_back);
	m_fire_mode							= UIHelper::CreateTextWnd( xml, "static_fire_mode", m_back);
	m_magnification						= UIHelper::CreateTextWnd( xml, "static_magnification", m_back);

	xml.SetLocalRoot( stored_root );
}

void CUIHudStatesWnd::on_connected()
{
	Load_section();
}

void CUIHudStatesWnd::Load_section()
{
	VERIFY( g_pGameLevel );
	if ( !Level().hud_zones_list )
	{
		Level().create_hud_zones_list();
		VERIFY( Level().hud_zones_list );
	}
	
//	m_actor_radia_factor = pSettings->r_float( "radiation_zone_detector", "actor_radia_factor" );
	Level().hud_zones_list->load( "all_zone_detector", "zone" );

	Load_section_type( ALife::infl_rad,     "radiation_zone_detector" );
	Load_section_type( ALife::infl_fire,    "fire_zone_detector" );
	Load_section_type( ALife::infl_acid,    "acid_zone_detector" );
	Load_section_type( ALife::infl_psi,     "psi_zone_detector" );
	Load_section_type( ALife::infl_electra, "electra_zone_detector" );	//no uistatic
}

void CUIHudStatesWnd::Load_section_type( ALife::EInfluenceType type, LPCSTR section )
{
	/*m_zone_max_power[type] = pSettings->r_float( section, "max_power" );
	if ( m_zone_max_power[type] <= 0.0f )
	{
		m_zone_max_power[type] = 1.0f;
	}*/
	m_zone_feel_radius[type] = pSettings->r_float( section, "zone_radius" );
	if ( m_zone_feel_radius[type] <= 0.0f )
	{
		m_zone_feel_radius[type] = 1.0f;
	}
	if ( m_zone_feel_radius_max < m_zone_feel_radius[type] )
	{
		m_zone_feel_radius_max = m_zone_feel_radius[type];
	}
	m_zone_threshold[type] = pSettings->r_float( section, "threshold" );
}

void CUIHudStatesWnd::Update()
{
	CActor* actor = smart_cast<CActor*>( Level().CurrentViewEntity() );
	if ( !actor )
	{
		return;
	}

	UpdateHealth( actor );
	UpdateActiveItemInfo( actor );
	UpdateIndicators( actor );
	
	UpdateZones();

	inherited::Update();
}

void CUIHudStatesWnd::UpdateHealth( CActor* actor )
{
	float cur_health = actor->GetfHealth();
	m_ui_health_bar->SetProgressPos(iCeil(cur_health * 100.0f * 35.f) / 35.f);
	if ( _abs(cur_health - m_last_health) > m_health_blink )
	{
		m_last_health = cur_health;
		m_ui_health_bar->m_UIProgressItem.ResetColorAnimation();
	}
	
	float cur_stamina = actor->conditions().GetPower();
	m_ui_stamina_bar->SetProgressPos(iCeil(cur_stamina * 100.0f * 35.f) / 35.f);
	if ( !actor->conditions().IsCantSprint() )
	{
		m_ui_stamina_bar->m_UIProgressItem.ResetColorAnimation();
	}
}

void CUIHudStatesWnd::UpdateActiveItemInfo(CActor* actor)
{
	auto ai								= actor->inventory().ActiveItem();
	if (auto wpn = (ai) ? ai->cast<CWeaponMagazined*>() : nullptr)
	{
		if (m_b_force_update)
		{
			wpn->ForceUpdateAmmo		();
			m_b_force_update			= false;
		}
		wpn->GetBriefInfo				(m_wpn_info);

		m_zeroing->SetText				(*m_wpn_info.zeroing);
		m_fire_mode->SetText			(*m_wpn_info.fire_mode);
		m_magnification->SetText		(*m_wpn_info.magnification);
		
		m_back->Show					(true);
	}
	else
		m_back->Show					(false);
}

// ------------------------------------------------------------------------------------------------
void CUIHudStatesWnd::UpdateZones()
{
	//float actor_radia = m_actor->conditions().GetRadiation() * m_actor_radia_factor;
	//m_radia_hit = _max( m_zone_cur_power[it_rad], actor_radia );

	CActor* actor = smart_cast<CActor*>( Level().CurrentViewEntity() );
	if ( !actor )
	{
		return;
	}
	CPda* const pda	= actor->GetPDA();
	if(pda)
	{
		typedef xr_vector<CObject*>	monsters;
		for(monsters::const_iterator it	= pda->feel_touch.begin();
									 it != pda->feel_touch.end(); ++it)
		{
			CBaseMonster* const	monster	= smart_cast<CBaseMonster*>(*it);
			if(!monster || !monster->g_Alive()) 
				continue;

			monster->play_detector_sound();
		}
	}


	m_radia_self = actor->conditions().GetRadiation();
	
	float zone_max_power = actor->conditions().GetZoneMaxPower(ALife::infl_rad);
	float power          = actor->conditions().GetInjuriousMaterialDamage();
	power = power / zone_max_power;
	clamp( power, 0.0f, 1.1f );
	if ( m_zone_cur_power[ALife::infl_rad] < power )
	{
		m_zone_cur_power[ALife::infl_rad] = power;
	}
	m_radia_hit = m_zone_cur_power[ALife::infl_rad];

/*	if ( Device.dwFrame % 20 == 0 )
	{
		Msg(" self = %.2f   hit = %.2f", m_radia_self, m_radia_hit );
	}*/

//	m_arrow->SetNewValue( m_radia_hit );
//	m_arrow_shadow->SetPos( m_arrow->GetPos() );
/*
	power = actor->conditions().GetPsy();
	clamp( power, 0.0f, 1.1f );
	if ( m_zone_cur_power[ALife::infl_psi] < power )
	{
		m_zone_cur_power[ALife::infl_psi] = power;
	}
*/
	if ( !Level().hud_zones_list )
	{
		return;
	}

	for ( int i = 0; i < ALife::infl_max_count; ++i )
	{
		if ( Device.fTimeDelta < 1.0f )
		{
			m_zone_cur_power[i] *= 0.9f * (1.0f - Device.fTimeDelta);
		}
		if ( m_zone_cur_power[i] < 0.01f )
		{
			m_zone_cur_power[i] = 0.0f;
		}
	}

	Fvector posf; 
	posf.set( Device.vCameraPosition );
	Level().hud_zones_list->feel_touch_update( posf, m_zone_feel_radius_max );
	
	if ( Level().hud_zones_list->m_ItemInfos.size() == 0 )
	{
		return;
	}

	CZoneList::ItemsMapIt itb	= Level().hud_zones_list->m_ItemInfos.begin();
	CZoneList::ItemsMapIt ite	= Level().hud_zones_list->m_ItemInfos.end();
	for ( ; itb != ite; ++itb ) 
	{
		CCustomZone*		pZone = itb->first;
		ITEM_INFO&			zone_info = itb->second;
		ITEM_TYPE*			zone_type = zone_info.curr_ref;
		
		ALife::EHitType			hit_type = pZone->GetHitType();
		ALife::EInfluenceType	z_type = get_indik_type( hit_type );
/*		if ( z_type == indik_type_max )
		{
			continue;
		}
*/

		Fvector P			= Device.vCameraPosition;
		P.y					-= 0.5f;
		float dist_to_zone	= 0.0f;
		float rad_zone		= 0.0f;
		pZone->CalcDistanceTo( P, dist_to_zone, rad_zone );
		clamp( dist_to_zone, 0.0f, flt_max * 0.5f );
		
		float fRelPow = ( dist_to_zone / (rad_zone + (z_type==ALife::infl_max_count)? 5.0f : m_zone_feel_radius[z_type] + 0.1f) ) - 0.1f;

		zone_max_power = actor->conditions().GetZoneMaxPower(z_type);
		power = pZone->Power( dist_to_zone, rad_zone );
		//power = power / zone_max_power;
		clamp( power, 0.0f, 1.1f );

		if ( (z_type!=ALife::infl_max_count) && (m_zone_cur_power[z_type] < power) ) //max
		{
			m_zone_cur_power[z_type] = power;
		}

		if ( dist_to_zone < rad_zone + 0.9f * ((z_type==ALife::infl_max_count)?5.0f:m_zone_feel_radius[z_type]) )
		{
			fRelPow *= 0.6f;
			if ( dist_to_zone < rad_zone )
			{
				fRelPow *= 0.3f;
				fRelPow *= ( 2.5f - 2.0f * power ); // ���� ������� �� ���� ����
			}
		}
		clamp( fRelPow, 0.0f, 1.0f );

		//���������� ������� ������� ������������ �������
		zone_info.cur_period = zone_type->freq.x + (zone_type->freq.y - zone_type->freq.x) * (fRelPow * fRelPow);
		
		//string256	buff_z;
		//xr_sprintf( buff_z, "zone %2.2f\n", zone_info.cur_period );
		//xr_strcat( buff, buff_z );
		if( zone_info.snd_time > zone_info.cur_period )
		{
			zone_info.snd_time = 0.0f;
			HUD_SOUND_ITEM::PlaySound( zone_type->detect_snds, Fvector().set(0,0,0), NULL, true, false );
		} 
		else
		{
			zone_info.snd_time += Device.fTimeDelta;
		}
	} // for itb
}

void CUIHudStatesWnd::UpdateIndicators( CActor* actor )
{
	if(m_fake_indicators_update)
		return;

	for ( int i = 0; i < it_max ; ++i ) // it_max = ALife::infl_max_count-1
	{
		UpdateIndicatorType( actor, (ALife::EInfluenceType)i );
	}
}

void CUIHudStatesWnd::UpdateIndicatorType( CActor* actor, ALife::EInfluenceType type )
{
	if ( type < ALife::infl_rad || ALife::infl_psi < type )
	{
		VERIFY2( 0, "Failed EIndicatorType for CStatic!" );
		return;
	}

/*	
	u32 c_white  = color_rgba( 255, 255, 255, 255 );
	u32 c_green  = color_rgba( 0, 255, 0, 255 );
	u32 c_yellow = color_rgba( 255, 255, 0, 255 );
	u32 c_red    = color_rgba( 255, 0, 0, 255 );
*/
	LPCSTR texture = "";
	string128 str;
	switch(type)
	{
		case ALife::infl_rad: texture = "ui_inGame2_triangle_Radiation_"; break;
		case ALife::infl_fire: texture = "ui_inGame2_triangle_Fire_"; break;
		case ALife::infl_acid: texture = "ui_inGame2_triangle_Biological_"; break;
		case ALife::infl_psi: texture = "ui_inGame2_triangle_Psy_"; break;
		default: NODEFAULT;
	}
	float           hit_power = m_zone_cur_power[type];
	ALife::EHitType hit_type  = m_zone_hit_type[type];
	
	CCustomOutfit* outfit = actor->GetOutfit();
	CHelmet* helmet = smart_cast<CHelmet*>(actor->inventory().ItemFromSlot(HELMET_SLOT));
	float protect = (outfit) ? outfit->GetHitTypeProtection( hit_type ) : 0.0f;
	protect += (helmet) ? helmet->GetHitTypeProtection(hit_type) : 0.0f;
	protect += actor->GetProtectionArtefacts(hit_type);

	if ( hit_power < EPS )
	{
		m_indik[type]->Show(false);
//		m_indik[type]->SetTextureColor( c_white );
//		SwitchLA( false, type );
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}

	m_indik[type]->Show(true);
	if ( hit_power <= protect )
	{
//		m_indik[type]->SetTextureColor( c_green );
//		SwitchLA( false, type );
		xr_sprintf(str, sizeof(str), "%s%s", texture, "green");
		texture = str;
		m_indik[type]->InitTexture(texture);
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}
	if ( hit_power - protect < m_zone_threshold[type] )
	{
//		m_indik[type]->SetTextureColor( c_yellow );
//		SwitchLA( false, type );
		xr_sprintf(str, sizeof(str), "%s%s", texture, "yellow");
		texture = str;
		m_indik[type]->InitTexture(texture);
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}
//	m_indik[type]->SetTextureColor( c_red );
//	SwitchLA( true, type );
	xr_sprintf(str, sizeof(str), "%s%s", texture, "red");
	texture = str;
	m_indik[type]->InitTexture(texture);
	VERIFY(actor->conditions().GetZoneMaxPower(hit_type));
	actor->conditions().SetZoneDanger((hit_power-protect)/actor->conditions().GetZoneMaxPower(hit_type), type);
}
/*
void CUIHudStatesWnd::SwitchLA( bool state, ALife::EInfluenceType type )
{
	if ( state == m_cur_state_LA[type] )
	{
		return;
	}

	if ( state )
	{
		m_indik[type]->SetColorAnimation( m_lanim_name.c_str(), LA_CYCLIC|LA_TEXTURECOLOR);
		m_cur_state_LA[type] = true;
	}
	else
	{
		m_indik[type]->SetColorAnimation( NULL, 0);//off
		m_cur_state_LA[type] = false;
	}
}
*/
float CUIHudStatesWnd::get_zone_cur_power( ALife::EHitType hit_type )
{
	ALife::EInfluenceType iz_type = get_indik_type( hit_type );
	if ( iz_type == ALife::infl_max_count )
	{
		return 0.0f;
	}
	return m_zone_cur_power[iz_type];
}

void CUIHudStatesWnd::DrawZoneIndicators()
{
	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if(!actor)
		return;

	UpdateIndicators(actor);

	if(m_indik[ALife::infl_rad]->IsShown())
		m_indik[ALife::infl_rad]->Draw();

	if(m_indik[ALife::infl_fire]->IsShown())
		m_indik[ALife::infl_fire]->Draw();

	if(m_indik[ALife::infl_acid]->IsShown())
		m_indik[ALife::infl_acid]->Draw();

	if(m_indik[ALife::infl_psi]->IsShown())
		m_indik[ALife::infl_psi]->Draw();
}

void CUIHudStatesWnd::FakeUpdateIndicatorType(u8 t, float power)
{
	ALife::EInfluenceType type = (ALife::EInfluenceType)t;
	if ( type < ALife::infl_rad || ALife::infl_psi < type )
	{
		VERIFY2( 0, "Failed EIndicatorType for CStatic!" );
		return;
	}

	CActor* actor = smart_cast<CActor*>( Level().CurrentViewEntity() );
	if(!actor)
		return;

	LPCSTR texture = "";
	string128 str;
	switch(type)
	{
		case ALife::infl_rad: texture = "ui_inGame2_triangle_Radiation_"; break;
		case ALife::infl_fire: texture = "ui_inGame2_triangle_Fire_"; break;
		case ALife::infl_acid: texture = "ui_inGame2_triangle_Biological_"; break;
		case ALife::infl_psi: texture = "ui_inGame2_triangle_Psy_"; break;
		default: NODEFAULT;
	}
	float           hit_power = power;
	ALife::EHitType hit_type  = m_zone_hit_type[type];
	
	CCustomOutfit* outfit = actor->GetOutfit();
	CHelmet* helmet = smart_cast<CHelmet*>(actor->inventory().ItemFromSlot(HELMET_SLOT));
	float protect = (outfit) ? outfit->GetHitTypeProtection( hit_type ) : 0.0f;
	protect += (helmet) ? helmet->GetHitTypeProtection(hit_type) : 0.0f;
	protect += actor->GetProtectionArtefacts(hit_type);

	float max_power = actor->conditions().GetZoneMaxPower( hit_type );
	protect = protect / max_power; // = 0..1

	if ( hit_power < EPS )
	{
		m_indik[type]->Show(false);
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}

	m_indik[type]->Show(true);
	if ( hit_power < protect )
	{
		xr_sprintf(str, sizeof(str), "%s%s", texture, "green");
		texture = str;
		m_indik[type]->InitTexture(texture);
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}
	if ( hit_power - protect < m_zone_threshold[type] )
	{
		xr_sprintf(str, sizeof(str), "%s%s", texture, "yellow");
		texture = str;
		m_indik[type]->InitTexture(texture);
		actor->conditions().SetZoneDanger( 0.0f, type );
		return;
	}
	xr_sprintf(str, sizeof(str), "%s%s", texture, "red");
	texture = str;
	m_indik[type]->InitTexture(texture);
	actor->conditions().SetZoneDanger( hit_power - protect, type );
}

void CUIHudStatesWnd::EnableFakeIndicators(bool enable)
{
	m_fake_indicators_update = enable;
}