#include "pch_script.h"
#include "inventory_item_object.h"

void CInventoryItemObject::OnStateSwitch o$(u32 S, u32 oldState)
{
	CHudItem::OnStateSwitch(S, oldState);

	switch (S)
	{
	case eShowing:
		PlayHUDMotion("anm_show", FALSE, S);
		break;
	case eHiding:
		if (oldState != eHiding)
			PlayHUDMotion("anm_hide", FALSE, S);
		break;
	case eIdle:
		PlayAnimIdle();
		break;
	}
}

void CInventoryItemObject::OnAnimationEnd o$(u32 state)
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

#include "Entity_alive.h"
#include "inventoryOwner.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"
void CInventoryItemObject::UpdateXForm o$()
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
