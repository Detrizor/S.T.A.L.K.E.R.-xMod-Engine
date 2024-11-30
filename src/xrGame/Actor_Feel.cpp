#include "stdafx.h"
#include "actor.h"
#include "weapon.h"
#include "inventory.h"
#include "UsableScriptObject.h"
#include "UIGameCustom.h"
#include "Grenade.h"
#include "ExplosiveRocket.h"
#include "game_cl_base.h"
#include "Level.h"

#include "ui/UIMainIngameWnd.h"

#include "../xrEngine/gamemtllib.h"
#include "../xrEngine/CameraBase.h"

#define PICKUP_INFO_COLOR 0xFFDDDDDD

static bool validate_object(CObject* obj)
{
	PIItem item = obj->scast<PIItem>();
	if (!item);
	else if (!obj->getVisible());
	else if (Level().m_feel_deny.is_object_denied(obj));
	else if (obj->H_Parent());
	else if (obj->getDestroy());
	else if (!item->CanTake());
	else if (smart_cast<CExplosiveRocket*>(obj));
	else if (item->BaseSlot() != BOLT_SLOT && smart_cast<CGrenade*>(item) && !smart_cast<CGrenade*>(item)->Useful());
	else if (item->BaseSlot() != BOLT_SLOT && smart_cast<CMissile*>(item) && !smart_cast<CMissile*>(item)->Useful());
	else return true;
	return false;
}

void CActor::feel_touch_new				(CObject* O)
{
	CPhysicsShellHolder* sh=smart_cast<CPhysicsShellHolder*>(O);
	if(sh&&sh->character_physics_support()) m_feel_touch_characters++;

}

void CActor::feel_touch_delete	(CObject* O)
{
	CPhysicsShellHolder* sh=smart_cast<CPhysicsShellHolder*>(O);
	if(sh&&sh->character_physics_support()) m_feel_touch_characters--;
}

bool CActor::feel_touch_contact		(CObject *O)
{
	CInventoryItem	*item = smart_cast<CInventoryItem*>(O);
	CInventoryOwner	*inventory_owner = smart_cast<CInventoryOwner*>(O);

	if (item && item->Useful() && !item->object().H_Parent()) 
		return true;

	if(inventory_owner && inventory_owner != smart_cast<CInventoryOwner*>(this))
	{
		//CPhysicsShellHolder* sh=smart_cast<CPhysicsShellHolder*>(O);
		//if(sh&&sh->character_physics_support()) m_feel_touch_characters++;
		return true;
	}

	return		(false);
}

bool CActor::feel_touch_on_contact	(CObject *O)
{
	CCustomZone	*custom_zone = smart_cast<CCustomZone*>(O);
	if (!custom_zone)
		return	(true);

	Fsphere		sphere;
	Center		(sphere.P);
	sphere.R	= 0.1f;
	if (custom_zone->inside(sphere))
		return	(true);

	return		(false);
}

ICF static BOOL info_trace_callback(collide::rq_result& result, LPVOID params)
{
	BOOL& bOverlaped	= *(BOOL*)params;
	if(result.O)
	{
		if (Level().CurrentEntity()==result.O)
		{ //ignore self-actor
			return			TRUE;
		}else
		{ //check obstacle flag
			if(result.O->spatial.type&STYPE_OBSTACLE)
				bOverlaped			= TRUE;

			return			TRUE;
		}
	}else
	{
		//получить треугольник и узнать его материал
		CDB::TRI* T		= Level().ObjectSpace.GetStaticTris()+result.element;
		if (GMLib.GetMaterialByIdx(T->material)->Flags.is(SGameMtl::flPassable)) 
			return TRUE;
	}	
	bOverlaped			= TRUE;
	return				FALSE;
}

void CActor::feel_sound_new(CObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power)
{
	if (who == this)
		m_snd_noise = _max(m_snd_noise, power);
}

void CActor::PickupModeUpdate_COD()
{
	if (!m_bPickupMode || m_pPersonWeLookingAt || Level().CurrentViewEntity() != this)
		return;

	CObject* item_to_pickup					= validate_object(m_pObjectWeLookingAt) ? m_pObjectWeLookingAt : NULL;
	if (!item_to_pickup)
	{
		CFrustum							frustum;
		frustum.CreateFromMatrix			(Device.camera.full_transform, FRUSTUM_P_LRTB|FRUSTUM_P_FAR);

		ISpatialResult.clear_not_free		();
		g_SpatialSpace->q_frustum			(ISpatialResult, 0, STYPE_COLLIDEABLE, frustum);

		float maxlen						= m_fPickupRadius;
		for (u32 it = 0, it_e = ISpatialResult.size(); it != it_e; it++)
		{
			ISpatial* spatial				= ISpatialResult[it];
			CObject* obj					= spatial->dcast_CObject();
			if (!CanPickItem(frustum, Device.camera.position, obj) || !validate_object(obj))
				continue;

			Fvector							A; 
			obj->Center						(A);
			if (m_fUseRadius < A.distance_to(cam_Active()->vPosition))
				continue;

			Fvector							B, tmp;
			tmp.sub							(A, cam_Active()->vPosition);
			B.mad							(cam_Active()->vPosition, cam_Active()->vDirection, tmp.dotproduct(cam_Active()->vDirection));
			float len						= B.distance_to(A);
			if (len < maxlen)
			{
				maxlen						= len;
				item_to_pickup				= obj;
			}
		}
	}

	if (item_to_pickup)
		Game().SendPickUpEvent				(ID(), item_to_pickup->ID());
}

void CActor::PickupModeUpdate()
{
	if (!m_bInfoDraw)
		return;

	feel_touch_update						(cam_Active()->vPosition, m_fPickupInfoRadius);
	CFrustum								frustum;
	frustum.CreateFromMatrix				(Device.camera.full_transform, FRUSTUM_P_LRTB|FRUSTUM_P_FAR);

	for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
	{
		if (CanPickItem(frustum, Device.camera.position, *it) && validate_object(*it))
			PickupInfoDraw					(*it);
	}
}

BOOL CActor::CanPickItem(const CFrustum& frustum, const Fvector& from, CObject* obj)
{						FALSE;
	BOOL bOverlaped							= FALSE;
	Fvector									dir, to; 
	obj->Center								(to);
	float range								= dir.sub(to,from).magnitude();
	if (range > 0.25f)
	{
		if (frustum.testSphere_dirty(to, obj->Radius()))
		{
			dir.div							(range);
			collide::ray_defs RD			(from, dir, range, CDB::OPT_CULL, collide::rqtBoth);
			VERIFY							(!fis_zero(RD.dir.square_magnitude()));
			RQR.r_clear						();
			Level().ObjectSpace.RayQuery	(RQR, RD, info_trace_callback, &bOverlaped, NULL, obj);
		}
	}
	return									!bOverlaped;
}

void CActor::PickupInfoDraw(CObject* object)
{
	LPCSTR draw_str = NULL;
	
	CInventoryItem* item = smart_cast<CInventoryItem*>(object);
	if(!item)		return;

	Fmatrix			res;
	res.mul			(Device.camera.full_transform,object->XFORM());
	Fvector4		v_res;
	Fvector			shift;

	draw_str = item->getName();
	shift.set(0,0,0);

	res.transform(v_res,shift);

	if (v_res.z < 0 || v_res.w < 0)	return;
	if (v_res.x < -1.f || v_res.x > 1.f || v_res.y<-1.f || v_res.y>1.f) return;

	float x = (1.f + v_res.x)/2.f * (Device.dwWidth);
	float y = (1.f - v_res.y)/2.f * (Device.dwHeight);

	UI().Font().pFontLetterica16Russian->SetAligment	(CGameFont::alCenter);
	UI().Font().pFontLetterica16Russian->SetColor		(PICKUP_INFO_COLOR);
	UI().Font().pFontLetterica16Russian->Out			(x,y,draw_str);
}

void CActor::VicinityUpdate()
{
	auto it								= m_vicinity.begin();
	while (it != m_vicinity.end())
	{
		if (!(*it)->O.H_Parent() && check_item(*it))
			++it;
		else
		{
			(*it)->onInventoryAction	();
			m_vicinity.erase			(it);
		}
	}
	
	ISpatialResultVicinity.clear		();
	Fvector								center;
	Center								(center);
	g_SpatialSpace->q_sphere			(ISpatialResultVicinity, 0, STYPE_COLLIDEABLE, center, m_fVicinityRadius);
	for (auto spatial : ISpatialResultVicinity)
	{
		CObject* obj					= spatial->dcast_CObject();
		if (validate_object(obj))
		{
			auto item					= obj->scast<PIItem>();
			if (check_item(item) && !m_vicinity.contains(item))
			{
				m_vicinity.push_back	(item);
				item->onInventoryAction	();
			}
		}
	}
}

bool CActor::check_item(PIItem item) const
{
	Fvector								A;
	item->object().Center				(A);
	float distance						= A.distance_to(cameras[cam_active]->vPosition);
	return								(distance - item->object().Radius() < m_fVicinityRadius);
}

void CActor::resetVicinity()
{
	m_vicinity.clear					();
}
