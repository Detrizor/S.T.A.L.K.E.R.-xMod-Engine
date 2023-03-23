////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item_object.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item object implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item_object.h"

CEatableItemObject::CEatableItemObject	()
{
}

CEatableItemObject::~CEatableItemObject	()
{
}

DLL_Pure *CEatableItemObject::_construct	()
{
	CEatableItem::_construct	();
	CPhysicItem::_construct		();
	CHudItem::_construct		();
	return						(this);
}

void CEatableItemObject::Load				(LPCSTR section) 
{
	CPhysicItem::Load			(section);
	CEatableItem::Load			(section);
	CHudItem::Load				(section);
}

bool CEatableItemObject::Action(u16 cmd, u32 flags)
{
	if (CEatableItem::Action	(cmd, flags))
		return					(true);
	return						(CHudItem::Action(cmd, flags));
}

void CEatableItemObject::SwitchState(u32 S)
{
	CHudItem::SwitchState		(S);
}

void CEatableItemObject::OnMoveToRuck(const SInvItemPlace& prev)
{
	CEatableItem::OnMoveToRuck			(prev);
	CHudItem::OnMoveToRuck				(prev);
}

void CEatableItemObject::on_renderable_Render()
{
	CEatableItem::renderable_Render();
}

void CEatableItemObject::OnStateSwitch(u32 S, u32 oldState)
{
	CHudItem::OnStateSwitch		(S, oldState);
	
	switch(S)
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

void CEatableItemObject::OnAnimationEnd(u32 state)
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

void CEatableItemObject::Show()
{
	SwitchState(eShowing);
}

void CEatableItemObject::Hide()
{
	SwitchState(eHiding);
}

void CEatableItemObject::OnActiveItem()
{
	SwitchState					(eShowing);
	CHudItem::OnActiveItem		();
}

void CEatableItemObject::OnHiddenItem()
{
	SwitchState					(eHiding);
	CHudItem::OnHiddenItem		();
}

#include "inventoryOwner.h"
#include "Entity_alive.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
void CEatableItemObject::UpdateXForm()
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

//void CEatableItemObject::Hit(float P, Fvector &dir,	
//						 CObject* who, s16 element,
//						 Fvector position_in_object_space, 
//						 float impulse, 
//						 ALife::EHitType hit_type)
void	CEatableItemObject::Hit					(SHit* pHDS)
{
	/*
	CPhysicItem::Hit			(
		P,
		dir,
		who,
		element,
		position_in_object_space,
		impulse,
		hit_type
	);
	
	CEatableItem::Hit			(
		P,
		dir,
		who,
		element,
		position_in_object_space,
		impulse,
		hit_type
	);
	*/
	CPhysicItem::Hit(pHDS);
	CEatableItem::Hit(pHDS);
}

void CEatableItemObject::OnH_A_Independent	()
{
	CHudItem::OnH_A_Independent			();
	CEatableItem::OnH_A_Independent		();
	CPhysicItem::OnH_A_Independent		();

	// If we are dropping used item before removing - don't show it
	if (!Useful())
	{
		setVisible(false);
		setEnabled(false);
	}
}

void CEatableItemObject::OnH_B_Independent	(bool just_before_destroy)
{
	CHudItem::OnH_B_Independent			(just_before_destroy);
	CEatableItem::OnH_B_Independent		(just_before_destroy);
	CPhysicItem::OnH_B_Independent		(just_before_destroy);
}

void CEatableItemObject::OnH_B_Chield	()
{
	CPhysicItem::OnH_B_Chield			();
	CEatableItem::OnH_B_Chield			();
	CHudItem::OnH_B_Chield				();
}

void CEatableItemObject::OnH_A_Chield	()
{
	CHudItem::OnH_A_Chield				();
	CPhysicItem::OnH_A_Chield			();
	CEatableItem::OnH_A_Chield			();
}

void CEatableItemObject::UpdateCL		()
{
	CPhysicItem::UpdateCL				();
	CEatableItem::UpdateCL				();
	CHudItem::UpdateCL					();
}

void CEatableItemObject::OnEvent		(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent				(P, type);
	CEatableItem::OnEvent				(P, type);
	CHudItem::OnEvent					(P, type);
}

BOOL CEatableItemObject::net_Spawn		(CSE_Abstract* DC)
{
	BOOL								res = CPhysicItem::net_Spawn(DC);
	CEatableItem::net_Spawn				(DC);
	CHudItem::net_Spawn					(DC);
	return								(res);
}

void CEatableItemObject::net_Destroy	()
{
	CHudItem::net_Destroy				();
	CEatableItem::net_Destroy			();
	CPhysicItem::net_Destroy			();
}

void CEatableItemObject::net_Import		(NET_Packet& P) 
{	
	CEatableItem::net_Import			(P);
}

void CEatableItemObject::net_Export		(NET_Packet& P) 
{	
	CEatableItem::net_Export			(P);
}

void CEatableItemObject::save			(NET_Packet &packet)
{
	CPhysicItem::save					(packet);
	CEatableItem::save					(packet);
}

void CEatableItemObject::load			(IReader &packet)
{
	CPhysicItem::load					(packet);
	CEatableItem::load					(packet);
}

void CEatableItemObject::renderable_Render()
{
	CPhysicItem::renderable_Render		();
	CEatableItem::renderable_Render		();
	CHudItem::renderable_Render			();
}

void CEatableItemObject::reload			(LPCSTR section)
{
	CPhysicItem::reload					(section);
	CEatableItem::reload				(section);
}

void CEatableItemObject::reinit			()
{
	CEatableItem::reinit				();
	CPhysicItem::reinit					();
}

void CEatableItemObject::activate_physic_shell	()
{
	CEatableItem::activate_physic_shell	();
}

void CEatableItemObject::on_activate_physic_shell()
{
	CPhysicItem::activate_physic_shell	();
}

void CEatableItemObject::make_Interpolation	()
{
	CEatableItem::make_Interpolation	();
}

void CEatableItemObject::PH_B_CrPr		()
{
	CEatableItem::PH_B_CrPr			();
}	

void CEatableItemObject::PH_I_CrPr		()
{
	CEatableItem::PH_I_CrPr			();
} 

#ifdef DEBUG
void CEatableItemObject::PH_Ch_CrPr		()
{
	CEatableItem::PH_Ch_CrPr			();
}
#endif

void CEatableItemObject::PH_A_CrPr		()
{
	CEatableItem::PH_A_CrPr			();
}

#ifdef DEBUG
void CEatableItemObject::OnRender			()
{
	CEatableItem::OnRender			();
}
#endif

bool CEatableItemObject::NeedToDestroyObject() const
{
	return CInventoryItem::NeedToDestroyObject();
}

u32	 CEatableItemObject::ef_weapon_type		() const
{
	return								(0);
}

bool CEatableItemObject::Useful				() const
{
	return			(CEatableItem::Useful());
}
