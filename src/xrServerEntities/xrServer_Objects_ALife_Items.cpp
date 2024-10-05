////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects items for ALife simulator
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_Items.h"
#include "clsid_game.h"
#include "object_broker.h"
#include "WeaponAmmo.h"

#ifndef XRGAME_EXPORTS
#	include "bone.h"
#else
#	include "../xrEngine/bone.h"
#	ifdef DEBUG
#		define PHPH_DEBUG
#	endif
#endif
#ifdef PHPH_DEBUG
#include "PHDebug.h"
#endif
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeInventoryItem
////////////////////////////////////////////////////////////////////////////
CSE_ALifeInventoryItem::CSE_ALifeInventoryItem(LPCSTR caSection)
{
	//текущее состояние вещи
	m_fCondition				= 1.0f;
	m_dwCost					= 0;

	m_fMass						= pSettings->r_float(caSection, "inv_weight");
	m_dwCost					= CInventoryItem::readBaseCost(caSection);

	if (pSettings->line_exist(caSection, "condition"))
		m_fCondition			= pSettings->r_float(caSection, "condition");

	if (pSettings->line_exist(caSection, "health_value"))
		m_iHealthValue			= pSettings->r_s32(caSection, "health_value");
	else
		m_iHealthValue			= 0;

	if (pSettings->line_exist(caSection, "food_value"))
		m_iFoodValue			= pSettings->r_s32(caSection, "food_value");
	else
		m_iFoodValue			= 0;

	m_fDeteriorationValue		= 0;

	m_last_update_time			= 0;

	State.quaternion.x			= 0.f;
	State.quaternion.y			= 0.f;
	State.quaternion.z			= 1.f;
	State.quaternion.w			= 0.f;

	State.angular_vel.set		(0.f,0.f,0.f);
	State.linear_vel.set		(0.f,0.f,0.f);

#ifdef XRGAME_EXPORTS
			m_freeze_time	= Device.dwTimeGlobal;
#else
			m_freeze_time	= 0;
#endif

	m_relevent_random.seed		(u32(CPU::GetCLK() & u32(-1)));
	freezed						= false;
}

CSE_Abstract *CSE_ALifeInventoryItem::init	()
{
	m_self						= smart_cast<CSE_ALifeObject*>(this);
	R_ASSERT					(m_self);
//	m_self->m_flags.set			(CSE_ALifeObject::flSwitchOffline,TRUE);
	return						(base());
}

CSE_ALifeInventoryItem::~CSE_ALifeInventoryItem	()
{
}

void CSE_ALifeInventoryItem::STATE_Write	(NET_Packet &tNetPacket)
{
	tNetPacket.w_float			(m_fCondition);
	save_data					(m_upgrades, tNetPacket);
	State.position				= base()->o_Position;
}

void CSE_ALifeInventoryItem::STATE_Read		(NET_Packet &tNetPacket, u16 size)
{
	u16 m_wVersion = base()->m_wVersion;
	if (m_wVersion > 52)
		tNetPacket.r_float		(m_fCondition);

	if (m_wVersion > 123)
	{
		load_data				(m_upgrades, tNetPacket);
	}

	State.position				= base()->o_Position;
}

static inline bool check (const u8 &mask, const u8 &test)
{
	return							(!!(mask & test));
}

const	u32		CSE_ALifeInventoryItem::m_freeze_delta_time		= 1000;
const	u32		CSE_ALifeInventoryItem::random_limit			= 120;		

//if TRUE, then object sends update packet
BOOL CSE_ALifeInventoryItem::Net_Relevant()
{
	if (base()->ID_Parent != u16(-1))
		return		FALSE;

	if (!freezed)
		return		TRUE;

#ifdef XRGAME_EXPORTS
	if (Device.dwTimeGlobal >= (m_freeze_time + m_freeze_delta_time))
		return		FALSE;
#endif

	if (!prev_freezed)
	{
		prev_freezed = true;	//i.e. freezed
		return		TRUE;
	}

	if (m_relevent_random.randI(random_limit))
		return		FALSE;

	return			TRUE;
}

void CSE_ALifeInventoryItem::UPDATE_Write	(NET_Packet &tNetPacket)
{
	tNetPacket.w_u8(0);
}

void CSE_ALifeInventoryItem::UPDATE_Read	(NET_Packet &tNetPacket)
{
	tNetPacket.r_u8();
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeInventoryItem::FillProps		(LPCSTR pref, PropItemVec& values)
{
	PHelper().CreateFloat			(values, PrepareKey(pref, *base()->s_name, "Item condition"), 		&m_fCondition, 			0.f, 1.f);
	CSE_ALifeObject					*alife_object = smart_cast<CSE_ALifeObject*>(base());
	R_ASSERT						(alife_object);
	PHelper().CreateFlag32			(values, PrepareKey(pref, *base()->s_name,"ALife\\Useful for AI"),	&alife_object->m_flags,	CSE_ALifeObject::flUsefulForAI);
	PHelper().CreateFlag32			(values, PrepareKey(pref, *base()->s_name,"ALife\\Visible for AI"),	&alife_object->m_flags,	CSE_ALifeObject::flVisibleForAI);
}
#endif // #ifndef XRGAME_EXPORTS

bool CSE_ALifeInventoryItem::bfUseful		()
{
	return						(true);
}

u32 CSE_ALifeInventoryItem::update_rate		() const
{
	return						(1000);
}

bool CSE_ALifeInventoryItem::has_upgrade( const shared_str& upgrade_id )
{
	return ( std::find( m_upgrades.begin(), m_upgrades.end(), upgrade_id ) != m_upgrades.end() );
}

void CSE_ALifeInventoryItem::add_upgrade( const shared_str& upgrade_id )
{
	if ( !has_upgrade( upgrade_id ) )
	{
		m_upgrades.push_back( upgrade_id );
		return;
	}
	FATAL( make_string( "Can`t add existent upgrade (%s)!", upgrade_id.c_str() ).c_str() );
}


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItem
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItem::CSE_ALifeItem				(LPCSTR caSection) : CSE_ALifeDynamicObjectVisual(caSection), CSE_ALifeInventoryItem(caSection)
{
	m_physics_disabled		= false;
}

CSE_ALifeItem::~CSE_ALifeItem				()
{
}

CSE_Abstract *CSE_ALifeItem::init			()
{
	inherited1::init			();
	inherited2::init			();
	return						(base());
}

CSE_Abstract *CSE_ALifeItem::base			()
{
	return						(inherited1::base());
}

const CSE_Abstract *CSE_ALifeItem::base		() const
{
	return						(inherited1::base());
}

void CSE_ALifeItem::STATE_Write(NET_Packet &tNetPacket)
{
	inherited1::STATE_Write				(tNetPacket);
	inherited2::STATE_Write				(tNetPacket);

	u16 mask							= 0;
	if (m_modules)
		for (u16 t = CSE_ALifeModule::mModuleTypesBegin; t < CSE_ALifeModule::mModuleTypesEnd; t++)
			if (m_modules[t])
				mask					|= (u16(1) << t);

	tNetPacket.w_u16					(mask);

	if (m_modules)
		for (u16 t = CSE_ALifeModule::mModuleTypesBegin; t < CSE_ALifeModule::mModuleTypesEnd; t++)
			if (m_modules[t])
				m_modules[t]->STATE_Write(tNetPacket);
}

void CSE_ALifeItem::STATE_Read(NET_Packet &tNetPacket, u16 size)
{
	inherited1::STATE_Read				(tNetPacket, size);
	if ((m_tClassID == CLSID_OBJECT_W_BINOCULAR) && (m_wVersion < 37))
	{
		tNetPacket.r_u16				();
		tNetPacket.r_u16				();
		tNetPacket.r_u8					();
	}
	inherited2::STATE_Read				(tNetPacket, size);
	
	if (m_wVersion < 129)
		return;

	u16									mask;
	tNetPacket.r_u16					(mask);
	for (u16 t = CSE_ALifeModule::mModuleTypesBegin; t < CSE_ALifeModule::mModuleTypesEnd; t++)
		if (mask & (u16(1) << t))
			add_module(static_cast<CSE_ALifeModule::eAlifeModuleTypes>(t))->STATE_Read(tNetPacket);
}

void CSE_ALifeItem::UPDATE_Write			(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
	inherited2::UPDATE_Write	(tNetPacket);

#ifdef XRGAME_EXPORTS
	m_last_update_time			= Device.dwTimeGlobal;
#endif // XRGAME_EXPORTS
};

void CSE_ALifeItem::UPDATE_Read				(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);

	m_physics_disabled			= false;
};

#ifndef XRGAME_EXPORTS
void CSE_ALifeItem::FillProps				(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps		(pref,	 values);
	inherited2::FillProps		(pref,	 values);
}
#endif // #ifndef XRGAME_EXPORTS

BOOL CSE_ALifeItem::Net_Relevant			()
{
	if (inherited1::Net_Relevant())
		return					(TRUE);

	if (inherited2::Net_Relevant())
		return					(TRUE);

	if (attached())
		return					(FALSE);

	if (!m_physics_disabled && !fis_zero(State.linear_vel.square_magnitude(),EPS_L))
		return					(TRUE);

#ifdef XRGAME_EXPORTS
//	if (Device.dwTimeGlobal < (m_last_update_time + update_rate()))
//		return					(FALSE);
#endif // XRGAME_EXPORTS

	return						(FALSE);
}

void CSE_ALifeItem::OnEvent					(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender )
{
	inherited1::OnEvent			(tNetPacket,type,time,sender);

	if (type != GE_FREEZE_OBJECT)
		return;

//	R_ASSERT					(!m_physics_disabled);
	m_physics_disabled			= true;
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemTorch
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemTorch::CSE_ALifeItemTorch		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_active					= false;
	m_nightvision_active		= false;
	m_attached					= false;
}

CSE_ALifeItemTorch::~CSE_ALifeItemTorch		()
{
}

BOOL	CSE_ALifeItemTorch::Net_Relevant			()
{
	if (m_attached) return true;
	return inherited::Net_Relevant();
}


void CSE_ALifeItemTorch::STATE_Read			(NET_Packet	&tNetPacket, u16 size)
{
	if (m_wVersion > 20)
		inherited::STATE_Read	(tNetPacket,size);

}

void CSE_ALifeItemTorch::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemTorch::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
	
	BYTE F = tNetPacket.r_u8();
	m_active					= !!(F & eTorchActive);
	m_nightvision_active		= !!(F & eNightVisionActive);
	m_attached					= !!(F & eAttached);
}

void CSE_ALifeItemTorch::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);

	BYTE F = 0;
	F |= (m_active ? eTorchActive : 0);
	F |= (m_nightvision_active ? eNightVisionActive : 0);
	F |= (m_attached ? eAttached : 0);
	tNetPacket.w_u8(F);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemTorch::FillProps			(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps			(pref,	 values);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeapon
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeapon::CSE_ALifeItemWeapon	(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_caAmmoSections			= pSettings->r_string(caSection,"ammo_class");
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));

	m_ef_main_weapon_type		= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_main_weapon_type",u32(-1));
	m_ef_weapon_type			= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));
}

u32	CSE_ALifeItemWeapon::ef_main_weapon_type() const
{
	VERIFY	(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CSE_ALifeItemWeapon::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemWeapon::STATE_Read(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);

	if (m_wVersion < 129)
		tNetPacket.r_u16		();

	if (m_wVersion < 133)
		tNetPacket.r_u16		();

	if (m_wVersion < 129)
		tNetPacket.r_u8			();
	
	if (m_wVersion > 40 && m_wVersion < 129)
		tNetPacket.r_u8			();

	if (m_wVersion > 46 && m_wVersion < 135)
		tNetPacket.r_u8			();
	
	if (m_wVersion > 122 && m_wVersion < 129)
		tNetPacket.r_u8			();

	if (m_wVersion >= 129 && m_wVersion < 135)
		tNetPacket.r_u8			();
}

u8	 CSE_ALifeItemWeapon::get_slot			()
{
	LPCSTR sl = pSettings->r_string(s_name, "slot");
	return						pSettings->r_u8("slot_ids", sl);
}

BOOL CSE_ALifeItemWeapon::Net_Relevant()
{
	if (wpn_flags==1)
		return TRUE;

	return inherited::Net_Relevant();
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemWeapon::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
	PHelper().CreateU8			(items,PrepareKey(pref,*s_name,"Ammo type:"), &ammo_type,0,255,1);
	PHelper().CreateU16			(items,PrepareKey(pref,*s_name,"Ammo: in magazine"),	&a_elapsed,0,30,1);
	

	if (m_scope_status == ALife::eAddonAttachable)
	       PHelper().CreateFlag8(items,PrepareKey(pref,*s_name,"Addons\\Scope"), 	&m_addon_flags, eWeaponAddonScope);

	if (m_silencer_status == ALife::eAddonAttachable)
        PHelper().CreateFlag8	(items,PrepareKey(pref,*s_name,"Addons\\Silencer"), 	&m_addon_flags, eWeaponAddonSilencer);

	if (m_grenade_launcher_status == ALife::eAddonAttachable)
        PHelper().CreateFlag8	(items,PrepareKey(pref,*s_name,"Addons\\Podstvolnik"),&m_addon_flags,eWeaponAddonGrenadeLauncher);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponAutoShotGun
////////////////////////////////////////////////////////////////////////////
void CSE_ALifeItemWeaponAutoShotGun::STATE_Read(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
	if (m_wVersion < 129 || m_wVersion >= 133)
		return;

	while (true)
	{
		if (P.r_u8() == u8_max)
			break;
		P.r_u8();
	}
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemWeaponAutoShotGun::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps(pref, items);
}
#endif // #ifndef XRGAME_EXPORTS
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponMagazined
////////////////////////////////////////////////////////////////////////////
void CSE_ALifeItemWeaponMagazined::STATE_Write(NET_Packet& P)
{
	inherited::STATE_Write(P);
	P.w_u8(m_u8CurFireMode);
	P.w_float(m_ads_shift);
	P.w_bool(m_locked);
	P.w_bool(m_cocked);
}

void CSE_ALifeItemWeaponMagazined::STATE_Read(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
	if (m_wVersion >= 129)
		P.r_u8(m_u8CurFireMode);
	if (m_wVersion >= 130)
		P.r_float(m_ads_shift);
	if (m_wVersion >= 132)
	{
		P.r_BOOL(m_locked);
		P.r_BOOL(m_cocked);
	}
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemWeaponMagazined::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponMagazinedWGL
////////////////////////////////////////////////////////////////////////////
void CSE_ALifeItemWeaponMagazinedWGL::STATE_Write(NET_Packet& P)
{
	inherited::STATE_Write(P);
	P.w_u8(m_bGrenadeMode);
}

void CSE_ALifeItemWeaponMagazinedWGL::STATE_Read(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
	if (m_wVersion >= 129)
		m_bGrenadeMode = P.r_u8();
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemWeaponMagazinedWGL::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemAmmo
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemAmmo::CSE_ALifeItemAmmo		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_boxSize					= CWeaponAmmo::readBoxSize(caSection);
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
		set_visual				(pSettings->r_string(caSection,"visual"));
}

void CSE_ALifeItemAmmo::STATE_Read			(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	tNetPacket.r_u16			(a_elapsed);

	if (m_wVersion >= 134)
		tNetPacket.r_u8			(m_mag_pos);
}

void CSE_ALifeItemAmmo::STATE_Write			(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u16			(a_elapsed);
	tNetPacket.w_u8				(m_mag_pos);
}

void CSE_ALifeItemAmmo::UPDATE_Read			(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
	tNetPacket.r_u16			(a_elapsed);
}

void CSE_ALifeItemAmmo::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
	tNetPacket.w_u16			(a_elapsed);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemAmmo::FillProps			(LPCSTR pref, PropItemVec& values) {
  	inherited::FillProps			(pref,values);
	PHelper().CreateU16			(values, PrepareKey(pref, *s_name, "Ammo: left"), &a_elapsed, 0, m_boxSize, m_boxSize);
}
#endif // #ifndef XRGAME_EXPORTS

bool CSE_ALifeItemAmmo::can_switch_offline	() const
{
	return ( inherited::can_switch_offline() && a_elapsed!=0 );
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemDetector
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemDetector::CSE_ALifeItemDetector(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_ef_detector_type	= pSettings->r_u32(caSection,"ef_detector_type");
}

CSE_ALifeItemDetector::~CSE_ALifeItemDetector()
{
}

u32	 CSE_ALifeItemDetector::ef_detector_type	() const
{
	return	(m_ef_detector_type);
}

void CSE_ALifeItemDetector::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	if (m_wVersion > 20)
		inherited::STATE_Read	(tNetPacket,size);
}

void CSE_ALifeItemDetector::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemDetector::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemDetector::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemDetector::FillProps		(LPCSTR pref, PropItemVec& items)
{
  	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemArtefact
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemArtefact::CSE_ALifeItemArtefact(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_fAnomalyValue				= 100.f;
}

CSE_ALifeItemArtefact::~CSE_ALifeItemArtefact()
{
}

void CSE_ALifeItemArtefact::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemArtefact::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemArtefact::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);	
}

void CSE_ALifeItemArtefact::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemArtefact::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
	PHelper().CreateFloat			(items, PrepareKey(pref, *s_name, "Anomaly value:"), &m_fAnomalyValue, 0.f, 200.f);
}
#endif // #ifndef XRGAME_EXPORTS

BOOL CSE_ALifeItemArtefact::Net_Relevant	()
{
	if (base()->ID_Parent == u16(-1))
		return TRUE;

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemPDA
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemPDA::CSE_ALifeItemPDA		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_original_owner		= 0xffff;
	m_specific_character	= NULL;
	m_info_portion			= NULL;
}


CSE_ALifeItemPDA::~CSE_ALifeItemPDA		()
{
}

void CSE_ALifeItemPDA::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	if (m_wVersion > 58)
		tNetPacket.r_u16		(m_original_owner);

	if (m_wVersion > 89)

	if ( (m_wVersion > 89)&&(m_wVersion < 98)  )
	{
		int tmp,tmp2;
		tNetPacket.r			(&tmp,		sizeof(int));
		tNetPacket.r			(&tmp2,		sizeof(int));
		m_info_portion			=	NULL;
		m_specific_character	= NULL;
	}else{
		tNetPacket.r_stringZ	(m_specific_character);
		tNetPacket.r_stringZ	(m_info_portion);
	
	}
}

void CSE_ALifeItemPDA::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u16				(m_original_owner);
#ifdef XRGAME_EXPORTS
	tNetPacket.w_stringZ		(m_specific_character);
	tNetPacket.w_stringZ		(m_info_portion);
#else
	shared_str		tmp_1	= NULL;
	shared_str						tmp_2	= NULL;

	tNetPacket.w_stringZ		(tmp_1);
	tNetPacket.w_stringZ		(tmp_2);
#endif

}

void CSE_ALifeItemPDA::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemPDA::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemPDA::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemDocument
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemDocument::CSE_ALifeItemDocument(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	m_wDoc					= NULL;
}

CSE_ALifeItemDocument::~CSE_ALifeItemDocument()
{
}

void CSE_ALifeItemDocument::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);

	if ( m_wVersion < 98  ){
		u16 tmp;
		tNetPacket.r_u16			(tmp);
		m_wDoc = NULL;
	}else
		tNetPacket.r_stringZ		(m_wDoc);
}

void CSE_ALifeItemDocument::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_stringZ		(m_wDoc);
}

void CSE_ALifeItemDocument::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemDocument::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemDocument::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
//	PHelper().CreateU16			(items, PrepareKey(pref, *s_name, "Document index :"), &m_wDocIndex, 0, 65535);
	PHelper().CreateRText		(items, PrepareKey(pref, *s_name, "Info portion :"), &m_wDoc);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemGrenade
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemGrenade::CSE_ALifeItemGrenade	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	m_ef_weapon_type	= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));
}

CSE_ALifeItemGrenade::~CSE_ALifeItemGrenade	()
{
}

u32	CSE_ALifeItemGrenade::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemGrenade::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemGrenade::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemGrenade::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemGrenade::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemGrenade::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemExplosive
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemExplosive::CSE_ALifeItemExplosive	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
}

CSE_ALifeItemExplosive::~CSE_ALifeItemExplosive	()
{
}

void CSE_ALifeItemExplosive::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemExplosive::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemExplosive::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemExplosive::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemExplosive::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemBolt
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemBolt::CSE_ALifeItemBolt		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_flags.set					(flUseSwitches,FALSE);
	m_flags.set					(flSwitchOffline,FALSE);
	m_ef_weapon_type			= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));
}

CSE_ALifeItemBolt::~CSE_ALifeItemBolt		()
{
}

u32	CSE_ALifeItemBolt::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemBolt::STATE_Write			(NET_Packet &tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemBolt::STATE_Read			(NET_Packet &tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);
}

void CSE_ALifeItemBolt::UPDATE_Write		(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Write	(tNetPacket);
};

void CSE_ALifeItemBolt::UPDATE_Read			(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
};

bool CSE_ALifeItemBolt::can_save			() const
{
	return						(false);//!attached());
}
bool CSE_ALifeItemBolt::used_ai_locations		() const
{
	return false;
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemBolt::FillProps			(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps			(pref,	 values);
}
#endif // #ifndef XRGAME_EXPORTS

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemCustomOutfit
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemCustomOutfit::CSE_ALifeItemCustomOutfit	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	m_ef_equipment_type		= pSettings->r_u32(caSection,"ef_equipment_type");
}

CSE_ALifeItemCustomOutfit::~CSE_ALifeItemCustomOutfit	()
{
}

u32	CSE_ALifeItemCustomOutfit::ef_equipment_type		() const
{
	return			(m_ef_equipment_type);
}

void CSE_ALifeItemCustomOutfit::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemCustomOutfit::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemCustomOutfit::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read			(tNetPacket);
}

void CSE_ALifeItemCustomOutfit::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write			(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemCustomOutfit::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

BOOL CSE_ALifeItemCustomOutfit::Net_Relevant		()
{
	return							(true);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemHelmet
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemHelmet::CSE_ALifeItemHelmet	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
}

CSE_ALifeItemHelmet::~CSE_ALifeItemHelmet	()
{
}

void CSE_ALifeItemHelmet::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemHelmet::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemHelmet::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read			(tNetPacket);
}

void CSE_ALifeItemHelmet::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write			(tNetPacket);
}

#ifndef XRGAME_EXPORTS
void CSE_ALifeItemHelmet::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}
#endif // #ifndef XRGAME_EXPORTS

BOOL CSE_ALifeItemHelmet::Net_Relevant		()
{
	return							(true);
}
