////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_object.cpp
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item object implementation
////////////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "pch_script.h"
#include "inventory_item_object.h"


CInventoryItemObjectOld::CInventoryItemObjectOld()
{
}

CInventoryItemObjectOld::~CInventoryItemObjectOld()
{
}

DLL_Pure *CInventoryItemObjectOld::_construct()
{
	CInventoryItem::_construct();
	CPhysicItem::_construct();
	return						(this);
}

void CInventoryItemObjectOld::Load(LPCSTR section)
{
	CPhysicItem::Load(section);
	CInventoryItem::Load(section);
}

void				CInventoryItemObjectOld::Hit(SHit* pHDS)
{
	CPhysicItem::Hit(pHDS);
	CInventoryItem::Hit(pHDS);
}

void CInventoryItemObjectOld::OnH_B_Independent(bool just_before_destroy)
{
	CInventoryItem::OnH_B_Independent(just_before_destroy);
	CPhysicItem::OnH_B_Independent(just_before_destroy);
}

void CInventoryItemObjectOld::OnH_A_Independent()
{
	CInventoryItem::OnH_A_Independent();
	CPhysicItem::OnH_A_Independent();
}


void CInventoryItemObjectOld::OnH_B_Chield()
{
	CPhysicItem::OnH_B_Chield();
	CInventoryItem::OnH_B_Chield();
}

void CInventoryItemObjectOld::OnH_A_Chield()
{
	CPhysicItem::OnH_A_Chield();
	CInventoryItem::OnH_A_Chield();
}

void CInventoryItemObjectOld::UpdateCL()
{
	CPhysicItem::UpdateCL();
	CInventoryItem::UpdateCL();
}

void CInventoryItemObjectOld::OnEvent(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent(P, type);
	CInventoryItem::OnEvent(P, type);
}

BOOL CInventoryItemObjectOld::net_Spawn(CSE_Abstract* DC)
{
	BOOL								res = CPhysicItem::net_Spawn(DC);
	CInventoryItem::net_Spawn(DC);
	return								(res);
}

void CInventoryItemObjectOld::net_Destroy()
{
	CInventoryItem::net_Destroy();
	CPhysicItem::net_Destroy();
}

void CInventoryItemObjectOld::net_Import(NET_Packet& P)
{
	CInventoryItem::net_Import(P);
}

void CInventoryItemObjectOld::net_Export(NET_Packet& P)
{
	CInventoryItem::net_Export(P);
}

void CInventoryItemObjectOld::save(NET_Packet &packet)
{
	CPhysicItem::save(packet);
	CInventoryItem::save(packet);
}

void CInventoryItemObjectOld::load(IReader &packet)
{
	CPhysicItem::load(packet);
	CInventoryItem::load(packet);
}

void CInventoryItemObjectOld::renderable_Render()
{
	CPhysicItem::renderable_Render();
	CInventoryItem::renderable_Render();
}

void CInventoryItemObjectOld::reload(LPCSTR section)
{
	CPhysicItem::reload(section);
	CInventoryItem::reload(section);
}

void CInventoryItemObjectOld::reinit()
{
	CInventoryItem::reinit();
	CPhysicItem::reinit();
}

void CInventoryItemObjectOld::activate_physic_shell()
{
	CInventoryItem::activate_physic_shell();
}

void CInventoryItemObjectOld::on_activate_physic_shell()
{
	CPhysicItem::activate_physic_shell();
}

void CInventoryItemObjectOld::make_Interpolation()
{
	CInventoryItem::make_Interpolation();
}

void CInventoryItemObjectOld::PH_B_CrPr()
{
	CInventoryItem::PH_B_CrPr();
}

void CInventoryItemObjectOld::PH_I_CrPr()
{
	CInventoryItem::PH_I_CrPr();
}

#ifdef DEBUG
void CInventoryItemObjectOld::PH_Ch_CrPr()
{
	CInventoryItem::PH_Ch_CrPr();
}
#endif

void CInventoryItemObjectOld::PH_A_CrPr()
{
	CInventoryItem::PH_A_CrPr();
}

#ifdef DEBUG
void CInventoryItemObjectOld::OnRender()
{
	CInventoryItem::OnRender();
}
#endif

void CInventoryItemObjectOld::modify_holder_params(float &range, float &fov) const
{
	CInventoryItem::modify_holder_params(range, fov);
}

u32	 CInventoryItemObjectOld::ef_weapon_type() const
{
	return								(0);
}

bool CInventoryItemObjectOld::NeedToDestroyObject() const
{
	return CInventoryItem::NeedToDestroyObject();
}

bool CInventoryItemObjectOld::Useful() const
{
	return			(CInventoryItem::Useful());
}

CInventoryItemObject::CInventoryItemObject()
{
}

CInventoryItemObject::~CInventoryItemObject()
{
}

DLL_Pure *CInventoryItemObject::_construct()
{
	CInventoryItem::_construct();
	CPhysicItem::_construct();
	CHudItem::_construct();
	return						(this);
}

bool CInventoryItemObject::Action(u16 cmd, u32 flags)
{
	if (CInventoryItem::Action(cmd, flags))
		return					(true);
	return						(CHudItem::Action(cmd, flags));
}

void CInventoryItemObject::on_renderable_Render()
{
	CPhysicItem::renderable_Render();
	CInventoryItem::renderable_Render();
}

void CInventoryItemObject::OnMoveToRuck(const SInvItemPlace& prev)
{
	CInventoryItem::OnMoveToRuck(prev);
	CHudItem::OnMoveToRuck(prev);
}

void CInventoryItemObject::SwitchState(u32 S)
{
	CHudItem::SwitchState(S);
}

void CInventoryItemObject::OnStateSwitch(u32 S, u32 oldState)
{
	CHudItem::OnStateSwitch(S, oldState);

	switch (S)
	{
	case eShowing:
		PlayHUDMotion("anm_show", FALSE, this, S);
		break;
	case eHiding:
		if (oldState != eHiding)
			PlayHUDMotion("anm_hide", FALSE, this, S);
		break;
	case eIdle:
		PlayAnimIdle();
		break;
	}
}

void CInventoryItemObject::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eHiding:
		SwitchState(eHidden);
		break;
	case eShowing:
	case eIdle:
		SwitchState(eIdle);
		break;
	default:
		CHudItem::OnAnimationEnd(state);
	}
}

void CInventoryItemObject::Show()
{
	SwitchState(eShowing);
}

void CInventoryItemObject::Hide()
{
	SwitchState(eHiding);
}

bool CInventoryItemObject::ActivateItem()
{
	return						CHudItem::ActivateItem();
}

void CInventoryItemObject::DeactivateItem()
{
	CHudItem::DeactivateItem();
}

void CInventoryItemObject::OnActiveItem()
{
	SwitchState(eShowing);
	CHudItem::OnActiveItem();
}

void CInventoryItemObject::OnHiddenItem()
{
	SwitchState(eHiding);
	CHudItem::OnHiddenItem();
}

#include "Entity_alive.h"
#include "inventoryOwner.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
void CInventoryItemObject::UpdateXForm()
{
	if (Device.dwFrame != dwXF_Frame)
	{
		dwXF_Frame = Device.dwFrame;

		if (0 == H_Parent())	return;

		// Get access to entity and its visual
		CEntityAlive*		E = smart_cast<CEntityAlive*>(H_Parent());

		if (!E)				return;

		const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
		if (parent && parent->use_simplified_visual())
			return;

		VERIFY(E);
		IKinematics*		V = smart_cast<IKinematics*>	(E->Visual());
		VERIFY(V);
		if (CAttachableItem::enabled())
			return;

		// Get matrices
		int					boneL = -1, boneR = -1, boneR2 = -1;
		E->g_WeaponBones(boneL, boneR, boneR2);
		if (boneR == -1)	return;

		boneL = boneR2;

		V->CalculateBones();
		Fmatrix& mL = V->LL_GetTransform(u16(boneL));
		Fmatrix& mR = V->LL_GetTransform(u16(boneR));

		// Calculate
		Fmatrix				mRes;
		Fvector				R, D, N;
		D.sub(mL.c, mR.c);	D.normalize_safe();
		R.crossproduct(mR.j, D);		R.normalize_safe();
		N.crossproduct(D, R);			N.normalize_safe();
		mRes.set(R, N, D, mR.c);
		mRes.mulA_43(E->XFORM());
		//		UpdatePosition		(mRes);
		XFORM().mul(mRes, offset());
	}
}

void CInventoryItemObject::Load(LPCSTR section)
{
	CPhysicItem::Load(section);
	CInventoryItem::Load(section);
	CHudItem::Load(section);
}

void CInventoryItemObject::Hit(SHit* pHDS)
{
	CPhysicItem::Hit(pHDS);
	CInventoryItem::Hit(pHDS);
}

void CInventoryItemObject::OnH_B_Independent(bool just_before_destroy)
{
	CHudItem::OnH_B_Independent(just_before_destroy);
	CInventoryItem::OnH_B_Independent(just_before_destroy);
	CPhysicItem::OnH_B_Independent(just_before_destroy);
}

void CInventoryItemObject::OnH_A_Independent()
{
	CHudItem::OnH_A_Independent();
	CInventoryItem::OnH_A_Independent();
	CPhysicItem::OnH_A_Independent();
}


void CInventoryItemObject::OnH_B_Chield()
{
	CPhysicItem::OnH_B_Chield();
	CInventoryItem::OnH_B_Chield();
	CHudItem::OnH_B_Chield();
}

void CInventoryItemObject::OnH_A_Chield()
{
	CHudItem::OnH_A_Chield();
	CPhysicItem::OnH_A_Chield();
	CInventoryItem::OnH_A_Chield();
}

void CInventoryItemObject::UpdateCL()
{
	CPhysicItem::UpdateCL();
	CInventoryItem::UpdateCL();
	CHudItem::UpdateCL();
}

void CInventoryItemObject::OnEvent(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent(P, type);
	CInventoryItem::OnEvent(P, type);
	CHudItem::OnEvent(P, type);
}

BOOL CInventoryItemObject::net_Spawn(CSE_Abstract* DC)
{
	BOOL								res = CPhysicItem::net_Spawn(DC);
	CInventoryItem::net_Spawn(DC);
	return								(res && CHudItem::net_Spawn(DC));
}

void CInventoryItemObject::net_Destroy()
{
	CHudItem::net_Destroy();
	CInventoryItem::net_Destroy();
	CPhysicItem::net_Destroy();
}

void CInventoryItemObject::net_Import(NET_Packet& P)
{
	CInventoryItem::net_Import(P);
}

void CInventoryItemObject::net_Export(NET_Packet& P)
{
	CInventoryItem::net_Export(P);
}

void CInventoryItemObject::save(NET_Packet &packet)
{
	CPhysicItem::save(packet);
	CInventoryItem::save(packet);
}

void CInventoryItemObject::load(IReader &packet)
{
	CPhysicItem::load(packet);
	CInventoryItem::load(packet);
}

void CInventoryItemObject::renderable_Render()
{
	CHudItem::renderable_Render();
}

void CInventoryItemObject::reload(LPCSTR section)
{
	CPhysicItem::reload(section);
	CInventoryItem::reload(section);
}

void CInventoryItemObject::reinit()
{
	CInventoryItem::reinit();
	CPhysicItem::reinit();
}

void CInventoryItemObject::activate_physic_shell()
{
	CInventoryItem::activate_physic_shell();
}

void CInventoryItemObject::on_activate_physic_shell()
{
	CPhysicItem::activate_physic_shell();
}

void CInventoryItemObject::make_Interpolation()
{
	CInventoryItem::make_Interpolation();
}

void CInventoryItemObject::PH_B_CrPr()
{
	CInventoryItem::PH_B_CrPr();
}

void CInventoryItemObject::PH_I_CrPr()
{
	CInventoryItem::PH_I_CrPr();
}

#ifdef DEBUG
void CInventoryItemObject::PH_Ch_CrPr()
{
	CInventoryItem::PH_Ch_CrPr();
}
#endif

void CInventoryItemObject::PH_A_CrPr()
{
	CInventoryItem::PH_A_CrPr();
}

#ifdef DEBUG
void CInventoryItemObject::OnRender()
{
	CInventoryItem::OnRender();
}
#endif

void CInventoryItemObject::modify_holder_params(float &range, float &fov) const
{
	CInventoryItem::modify_holder_params(range, fov);
}

u32	 CInventoryItemObject::ef_weapon_type() const
{
	return								(0);
}

bool CInventoryItemObject::NeedToDestroyObject() const
{
	return CInventoryItem::NeedToDestroyObject();
}

bool CInventoryItemObject::Useful() const
{
	return			(CInventoryItem::Useful());
}