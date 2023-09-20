#include "stdafx.h"
#include "actor.h"
#include "weapon.h"
#include "mercuryball.h"
#include "inventory.h"
#include "character_info.h"
#include "xr_level_controller.h"
#include "UsableScriptObject.h"
#include "customzone.h"
#include "../xrEngine/gamemtllib.h"
#include "ui/UIMainIngameWnd.h"
#include "UIGameCustom.h"
#include "Grenade.h"
#include "WeaponRPG7.h"
#include "ExplosiveRocket.h"
#include "game_cl_base.h"
#include "Level.h"
#include "clsid_game.h"
#include "hudmanager.h"
#include "ui/UIActorMenu.h"
#include "ui/UIDragDropListEx.h"
#include "ui/UICellItemFactory.h"
#include "ui/UICellItem.h"
#include "ui/UICellCustomItems.h"

#define PICKUP_INFO_COLOR 0xFFDDDDDD

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

bool CheckCellVicinity(Fvector center, float radius, CUIDragDropListEx* vicinity, CUICellItem* ci)
{
	if (PIItem(ci->m_pData)->object().Position().distance_to(center) < radius)
		return					true;
	CUICellItem* dying_cell		= vicinity->RemoveItem(ci, false);
	xr_delete					(dying_cell);
	return						false;
}

bool ValidateItem(PIItem item)
{
	if (!item)																	return false;
	if (item->object().H_Parent())												return false;
	if (item->object().getDestroy())											return false;
	if (!item->CanTake())														return false;
	if (smart_cast<CExplosiveRocket*>(&item->object()))							return false;
	if (item->BaseSlot() != BOLT_SLOT)
	{
		CGrenade* pGrenade		= smart_cast<CGrenade*>(item);
		if (pGrenade && !pGrenade->Useful())									return false;
		CMissile* pMissile		= smart_cast<CMissile*>(item);
		if (pMissile && !pMissile->Useful())									return false;
	}
	return						true;
}

bool FindItemInList(CUIDragDropListEx* lst, PIItem pItem)
{
	for (u32 i = 0, count = lst->ItemsCount(); i < count; ++i)
	{
		CUICellItem* ci = lst->GetItemIdx(i);
		for (u32 j = 0, ccount = ci->ChildsCount(); j < ccount; ++j)
		{
			if ((PIItem)ci->Child(j)->m_pData == pItem)
				return true;
		}

		if ((PIItem)ci->m_pData == pItem)
			return true;
	}
	return false;
}

void CActor::VicinityUpdate()
{
	CUIActorMenu& actor_menu				= CurrentGameUI()->GetActorMenu();
	if (actor_menu.GetMenuMode() == mmUndefined)
		return;
	
	Fvector									center;
	Center									(center);
	CUIDragDropListEx* vicinity				= actor_menu.m_pTrashList;
	for (u32 i = 0; i < vicinity->ItemsCount(); ++i)
	{
		CUICellItem* cell_item				= vicinity->GetItemIdx(i);
		for (u32 j = 0; j < cell_item->ChildsCount(); ++j)
		{
			if (!CheckCellVicinity(center, m_fVicinityRadius, vicinity, cell_item->Child(j)))
				j--;
		}
		if (!CheckCellVicinity(center, m_fVicinityRadius, vicinity, cell_item))
			i--;
	}

	ISpatialResultVicinity.clear_not_free	();
	g_SpatialSpace->q_sphere				(ISpatialResultVicinity, 0, STYPE_COLLIDEABLE, center, m_fVicinityRadius);

	for (u32 it = 0, it_e = ISpatialResultVicinity.size(); it < it_e; it++)
	{
		ISpatial*							spatial = ISpatialResultVicinity[it];
		CObject*							pObj = spatial->dcast_CObject();
		CInventoryItem*						pIItem = smart_cast<CInventoryItem*>(pObj);

		if (!ValidateItem(pIItem))
			continue;

		if ((pObj->Position().distance_to(center) < m_fVicinityRadius) && !FindItemInList(vicinity, pIItem))
			vicinity->SetItem				(create_cell_item(pIItem));
	}
}

#include "../xrEngine/CameraBase.h"

void CActor::PickupModeUpdate_COD()
{
	if (m_pPersonWeLookingAt || Level().CurrentViewEntity() != this)
		return;

	CFrustum						frustum;
	frustum.CreateFromMatrix		(Device.mFullTransform, FRUSTUM_P_LRTB|FRUSTUM_P_FAR);

	ISpatialResult.clear_not_free	();
	g_SpatialSpace->q_frustum		(ISpatialResult, 0, STYPE_COLLIDEABLE, frustum);

	float maxlen					= m_fVicinityRadius;
	CInventoryItem* pNearestItem	= NULL;

	for (u32 it = 0, it_e = ISpatialResult.size(); it != it_e; it++)
	{
		ISpatial* spatial			= ISpatialResult[it];
		CInventoryItem*	pIItem		= smart_cast<CInventoryItem*>(spatial->dcast_CObject());

		if (!ValidateItem(pIItem))
			continue;
		
		Fvector						A; 
		pIItem->object().Center		(A);
		if (A.distance_to_sqr(Position()) > 4)
			continue;

		Fvector						B, tmp;
		tmp.sub						(A, cam_Active()->vPosition);
		B.mad						(cam_Active()->vPosition, cam_Active()->vDirection, tmp.dotproduct(cam_Active()->vDirection));
		float len					= B.distance_to_sqr(A);
		if (len < maxlen)
		{
			maxlen					= len;
			pNearestItem			= pIItem;
		}
	}

	if (pNearestItem)
	{
		CFrustum					frustum;
		frustum.CreateFromMatrix	(Device.mFullTransform,FRUSTUM_P_LRTB|FRUSTUM_P_FAR);
		CGameObject* go				= pNearestItem->cast_game_object();
		if (!CanPickItem(frustum, Device.vCameraPosition, &pNearestItem->object()) || (go && (Level().m_feel_deny.is_object_denied(go) || !go->getVisible())))
			pNearestItem			= NULL;
	}

	if (pNearestItem && m_bPickupMode && CurrentGameUI()->GetActorMenu().GetMenuMode() == mmUndefined)
	{
		CUsableScriptObject*		pUsableObject = smart_cast<CUsableScriptObject*>(pNearestItem);
		if(pUsableObject && (!m_pUsableObject))
			pUsableObject->use		(this);

		//подбирание объекта
		Game().SendPickUpEvent		(ID(), pNearestItem->object().ID());
	}
}

void CActor::PickupModeUpdate()
{
	if (!m_bInfoDraw)
		return;

	feel_touch_update(Position(), m_fVicinityRadius);

	CFrustum frustum;
	frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

	for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
	{
		if (CanPickItem(frustum, Device.vCameraPosition, *it))
			PickupInfoDraw(*it);
	}
}

BOOL CActor::CanPickItem(const CFrustum& frustum, const Fvector& from, CObject* item)
{
	if(!item->getVisible())
		return FALSE;

	BOOL	bOverlaped		= FALSE;
	Fvector dir,to; 
	item->Center			(to);
	float range				= dir.sub(to,from).magnitude();
	if (range>0.25f)
	{
		if (frustum.testSphere_dirty(to,item->Radius()))
		{
			dir.div						(range);
			collide::ray_defs			RD(from, dir, range, CDB::OPT_CULL, collide::rqtBoth);
			VERIFY						(!fis_zero(RD.dir.square_magnitude()));
			RQR.r_clear					();
			Level().ObjectSpace.RayQuery(RQR, RD, info_trace_callback, &bOverlaped, NULL, item);
		}
	}
	return !bOverlaped;
}

void CActor::PickupInfoDraw(CObject* object)
{
	LPCSTR draw_str = NULL;
	
	CInventoryItem* item = smart_cast<CInventoryItem*>(object);
	if(!item)		return;

	Fmatrix			res;
	res.mul			(Device.mFullTransform,object->XFORM());
	Fvector4		v_res;
	Fvector			shift;

	draw_str = item->NameItem();
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