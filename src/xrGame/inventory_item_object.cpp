#include "pch_script.h"
#include "inventory_item_object.h"
#include "Entity_alive.h"
#include "inventoryOwner.h"
#include "../Include/xrRender/Kinematics.h"
#include "../Include/xrRender/KinematicsAnimated.h"

void CInventoryItemObject::UpdateXForm()
{
	if (Device.dwFrame == dwXF_Frame)
		return;
	dwXF_Frame							= Device.dwFrame;

	if (!H_Parent())
		return;

	// Get access to entity and its visual
	CEntityAlive* E						= smart_cast<CEntityAlive*>(H_Parent());
	if (!E)
		return;

	auto parent							= E->scast<CInventoryOwner*>();
	if (!parent || parent->use_simplified_visual())
		return;

	if (parent->attached(this) || CAttachableItem::enabled())
		return;

	VERIFY								(E);
	IKinematics* V						= smart_cast<IKinematics*>(E->Visual());
	VERIFY								(V);

	// Get matrices
	int boneL							= -1;
	int boneR							= -1;
	int boneR2							= -1;
	E->g_WeaponBones					(boneL, boneR, boneR2);
	if (boneR == -1)
		return;

	boneL								= boneR2;

	V->CalculateBones					();
	Fmatrix& mL							= V->LL_GetTransform(u16(boneL));
	Fmatrix& mR							= V->LL_GetTransform(u16(boneR));

	// Calculate
	Fmatrix								mRes;
	Fvector								R, D, n;
	D.sub								(mL.c, mR.c);
	D.normalize_safe					();
	R.crossproduct						(mR.j, D);
	R.normalize_safe					();
	n.crossproduct						(D, R);
	n.normalize_safe					();

	mRes.set							(R, n, D, mR.c);
	mRes.mulA_43						(E->XFORM());
	XFORM().mul							(mRes, offset());
}

void CInventoryItemObject::renderable_Render()
{
	UpdateXForm							();
	__super::renderable_Render			();
}

void CInventoryItemObject::shedule_Update(u32 T)
{
	core::shedule_Update				(T);
	wrap::shedule_Update				(T);
	if (auto parent = H_Parent())
		if (dwXF_Frame < Device.dwFrame - 10)
			XFORM().c					= parent->Position();
}
