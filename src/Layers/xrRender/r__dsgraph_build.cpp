#include "stdafx.h"

#include "fhierrarhyvisual.h"
#include "SkeletonCustom.h"
#include "../../xrEngine/fmesh.h"
#include "../../xrEngine/irenderable.h"

#include "flod.h"
#include "particlegroup.h"
#include "FTreeVisual.h"

using	namespace R_dsgraph;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Scene graph actual insertion and sorting ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

float		r_ssaDISCARD;
float		r_ssaDONTSORT;
float		r_ssaLOD_A,			r_ssaLOD_B;
float		r_ssaGLOD_start,	r_ssaGLOD_end;
float		r_ssaHZBvsTEX;

ICF	float	CalcSSA				(float& distSQ, Fvector CR$ C, dxRender_Visual* V)
{
	float R	= V->vis.sphere.R + 0;
	distSQ	= Device.camera.position.distance_to_sqr(C)+EPS;
	return	R/distSQ;
}

ICF	float	CalcSSA				(float& distSQ, Fvector CR$ C, float R)
{
	distSQ	= Device.camera.position.distance_to_sqr(C)+EPS;
	return	R/distSQ;
}

void R_dsgraph_structure::r_dsgraph_insert_dynamic(dxRender_Visual *pVisual, Fvector CR$ Center)
{
	CRender&	RI			=	RImplementation;

	if (pVisual->vis.marker	==	RI.marker)	return	;
	pVisual->vis.marker		=	RI.marker			;

	float distSQ			;
	float SSA				=	CalcSSA		(distSQ, Center, pVisual);
	if (SSA<=r_ssaDISCARD)		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY						(pVisual->shader._get());
	ShaderElement*		sh_d	= &*pVisual->shader->E[4];
	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority/2]) {
		mapSorted_Node* N		= mapDistort.insertInAnyWay	(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= RI.val_pObject;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= *RI.val_pTransform;
		N->val.se				= sh_d;		// 4=L_special
	}

	// Select shader
	ShaderElement*	sh		=	RImplementation.rimp_select_sh_dynamic	(pVisual,distSQ);
	if (0==sh)								return;
	if (!pmask[sh->flags.iPriority/2])		return;

	// Create common node
	// NOTE: Invisible elements exist only in R1
	_MatrixItem		item	= {SSA,RI.val_pObject,pVisual,*RI.val_pTransform};
	
	// HUD rendering
	if (RI.val_bHUD)
	{
		if (sh->flags.bStrictB2F)
		{
			if (sh->flags.bEmissive)
			{
				mapSorted_Node* N = mapHUDEmissive.insertInAnyWay(distSQ);
				N->val.ssa = SSA;
				N->val.pObject = RI.val_pObject;
				N->val.pVisual = pVisual;
				N->val.Matrix = *RI.val_pTransform;
				N->val.se = &*pVisual->shader->E[4]; // 4=L_special
			}
			mapSorted_Node* N = mapHUDSorted.insertInAnyWay(distSQ);
			N->val.ssa = SSA;
			N->val.pObject = RI.val_pObject;
			N->val.pVisual = pVisual;
			N->val.Matrix = *RI.val_pTransform;
			N->val.se = sh;
			return;
		}
		else
		{
			mapHUD_Node* N = mapHUD.insertInAnyWay(distSQ);
			N->val.ssa = SSA;
			N->val.pObject = RI.val_pObject;
			N->val.pVisual = pVisual;
			N->val.Matrix = *RI.val_pTransform;
			N->val.se = sh;
			if (sh->flags.bEmissive)
			{
				mapSorted_Node* N = mapHUDEmissive.insertInAnyWay(distSQ);
				N->val.ssa = SSA;
				N->val.pObject = RI.val_pObject;
				N->val.pVisual = pVisual;
				N->val.Matrix = *RI.val_pTransform;
				N->val.se = &*pVisual->shader->E[4]; // 4=L_special
			}
			return;
		}
	}

	// Shadows registering
	if (RI.val_bInvisible)		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F)	{
		mapSorted_Node* N		= mapSorted.insertInAnyWay	(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= RI.val_pObject;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= *RI.val_pTransform;
		N->val.se				= sh;
		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive) {
		mapSorted_Node* N		= mapEmissive.insertInAnyWay	(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= RI.val_pObject;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= *RI.val_pTransform;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}
	if (sh->flags.bWmark	&& pmask_wmark)	{
		mapSorted_Node* N		= mapWmark.insertInAnyWay		(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= RI.val_pObject;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= *RI.val_pTransform;
		N->val.se				= sh;							
		return					;
	}

	for ( u32 iPass = 0; iPass<sh->passes.size(); ++iPass)
	{
		// the most common node
		//SPass&						pass	= *sh->passes.front	();
		//mapMatrix_T&				map		= mapMatrix			[sh->flags.iPriority/2];
		SPass&						pass	= *sh->passes[iPass];
		mapMatrix_T&				map		= mapMatrixPasses	[sh->flags.iPriority/2][iPass];
		

#ifdef USE_RESOURCE_DEBUGGER
		mapMatrixVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapMatrixGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs);
		mapMatrixPS::TNode*			Nps		= Ngs->val.insert	(pass.ps);
		Nps->val.hs = pass.hs;
		Nps->val.ds = pass.ds;
		mapMatrixCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#else
		mapMatrixVS::TNode*			Nvs		= map.insert		(&*pass.vs);
		mapMatrixGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs->gs);
		mapMatrixPS::TNode*			Nps		= Ngs->val.insert	(pass.ps->ps);
		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;
		mapMatrixCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#endif

		mapMatrixStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapMatrixTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());
		mapMatrixItems&				items	= Ntex->val;
		items.push_back						(item);

		// Need to sort for HZB efficient use
		if (SSA>Ntex->val.ssa)		{ Ntex->val.ssa = SSA;
		if (SSA>Nstate->val.ssa)	{ Nstate->val.ssa = SSA;
		if (SSA>Ncs->val.ssa)		{ Ncs->val.ssa = SSA;
		if (SSA>Nps->val.mapCS.ssa)		{ Nps->val.mapCS.ssa = SSA;
		if (SSA>Ngs->val.ssa)		{ Ngs->val.ssa = SSA;
		if (SSA>Nvs->val.ssa)		{ Nvs->val.ssa = SSA;
		} } } } } }
	}

	if (val_recorder)			{
		Fbox3		temp		;
		Fmatrix CR$	xf			= *RI.val_pTransform;
		temp.xform	(pVisual->vis.box,xf);
		val_recorder->push_back	(temp);
	}
}

void R_dsgraph_structure::r_dsgraph_insert_static	(dxRender_Visual *pVisual)
{
	CRender&	RI				=	RImplementation;

	if (pVisual->vis.marker		==	RI.marker)	return	;
	pVisual->vis.marker			=	RI.marker			;

	float distSQ;
	float SSA					=	CalcSSA		(distSQ,pVisual->vis.sphere.P,pVisual);
	if (SSA<=r_ssaDISCARD)		return;

	// Distortive geometry should be marked and R2 special-cases it
	// a) Allow to optimize RT order
	// b) Should be rendered to special distort buffer in another pass
	VERIFY						(pVisual->shader._get());
	ShaderElement*		sh_d	= &*pVisual->shader->E[4];
	if (RImplementation.o.distortion && sh_d && sh_d->flags.bDistort && pmask[sh_d->flags.iPriority/2]) {
		mapSorted_Node* N		= mapDistort.insertInAnyWay		(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}

	// Select shader
	ShaderElement*		sh		= RImplementation.rimp_select_sh_static(pVisual,distSQ);
	if (0==sh)								return;
	if (!pmask[sh->flags.iPriority/2])		return;

	// strict-sorting selection
	if (sh->flags.bStrictB2F) {
		mapSorted_Node* N			= mapSorted.insertInAnyWay(distSQ);
		N->val.pObject				= NULL;
		N->val.pVisual				= pVisual;
		N->val.Matrix				= Fidentity;
		N->val.se					= sh;
		return;
	}

	// Emissive geometry should be marked and R2 special-cases it
	// a) Allow to skeep already lit pixels
	// b) Allow to make them 100% lit and really bright
	// c) Should not cast shadows
	// d) Should be rendered to accumulation buffer in the second pass
	if (sh->flags.bEmissive) {
		mapSorted_Node* N		= mapEmissive.insertInAnyWay	(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= &*pVisual->shader->E[4];		// 4=L_special
	}
	if (sh->flags.bWmark	&& pmask_wmark)	{
		mapSorted_Node* N		= mapWmark.insertInAnyWay		(distSQ);
		N->val.ssa				= SSA;
		N->val.pObject			= NULL;
		N->val.pVisual			= pVisual;
		N->val.Matrix			= Fidentity;
		N->val.se				= sh;							
		return					;
	}

	if	(val_feedback && counter_S==val_feedback_breakp)	val_feedback->rfeedback_static(pVisual);

	counter_S					++;

	for ( u32 iPass = 0; iPass<sh->passes.size(); ++iPass)
	{
		//SPass&						pass	= *sh->passes.front	();
		//mapNormal_T&				map		= mapNormal			[sh->flags.iPriority/2];
		SPass&						pass	= *sh->passes[iPass];
		mapNormal_T&				map		= mapNormalPasses[sh->flags.iPriority/2][iPass];

#ifdef USE_RESOURCE_DEBUGGER
		mapNormalVS::TNode*			Nvs		= map.insert		(pass.vs);
		mapNormalGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs);
		mapNormalPS::TNode*			Nps		= Ngs->val.insert	(pass.ps);
		Nps->val.hs = pass.hs;
		Nps->val.ds = pass.ds;
		mapNormalCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#else // USE_RESOURCE_DEBUGGER
		mapNormalVS::TNode*			Nvs		= map.insert		(&*pass.vs);
		mapNormalGS::TNode*			Ngs		= Nvs->val.insert	(pass.gs->gs);
		mapNormalPS::TNode*			Nps		= Ngs->val.insert	(pass.ps->ps);
		Nps->val.hs = pass.hs->sh;
		Nps->val.ds = pass.ds->sh;
		mapNormalCS::TNode*			Ncs		= Nps->val.mapCS.insert	(pass.constants._get());
#endif // USE_RESOURCE_DEBUGGER

		mapNormalStates::TNode*		Nstate	= Ncs->val.insert	(pass.state->state);
		mapNormalTextures::TNode*	Ntex	= Nstate->val.insert(pass.T._get());
		mapNormalItems&				items	= Ntex->val;
		_NormalItem					item	= {SSA,pVisual};
		items.push_back						(item);

		// Need to sort for HZB efficient use
		if (SSA>Ntex->val.ssa)		{ Ntex->val.ssa = SSA;
		if (SSA>Nstate->val.ssa)	{ Nstate->val.ssa = SSA;
		if (SSA>Ncs->val.ssa)		{ Ncs->val.ssa = SSA;
		if (SSA>Nps->val.mapCS.ssa)		{ Nps->val.mapCS.ssa = SSA;
		if (SSA>Ngs->val.ssa)		{ Ngs->val.ssa = SSA;
		if (SSA>Nvs->val.ssa)		{ Nvs->val.ssa = SSA;
		} } } } } }
	}

	if (val_recorder)
		val_recorder->push_back		(pVisual->vis.box);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRender::add_Dynamic(dxRender_Visual* pVisual, u32 planes)
{
	xoptional<Fvector>					tpos;
	auto Tpos = [&]() -> Fvector CR$
	{
		if (!tpos)
			val_pTransform->transform_tiny(tpos.getRef(true), pVisual->vis.sphere.P);
		return							tpos.getRef();
	};

	if (planes != u32_max)
	{
		// Check frustum visibility and calculate distance to visual's center
		EFC_Visible VIS					= View->testSphere(Tpos(), pVisual->vis.sphere.R, planes);
		if (VIS == fcvNone)
			return;
		if (VIS == fcvFully)	//further tests unnecessary
			planes						= u32_max;
	}

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		auto pG							= reinterpret_cast<PS::CParticleGroup*>(pVisual);
		for (auto& I : pG->items)
		{
			if (I._effect)
				add_Dynamic				(I._effect, planes);
			for (auto C : I._children_related)
				add_Dynamic				(C, planes);
			for (auto C : I._children_free)
				add_Dynamic				(C, planes);
		}
		break;
	}
	case MT_HIERRARHY:
	{
		auto pV							= reinterpret_cast<FHierrarhyVisual*>(pVisual);
		for (auto C : pV->children)
			add_Dynamic					(C, planes);
		break;
	}
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		auto pV							= reinterpret_cast<CKinematics*>(pVisual);
		BOOL _use_lod					= FALSE	;
		if (pV->m_lod)				
		{
			float						D;
			float ssa					= CalcSSA(D, Tpos(), pV->vis.sphere.R / 2.f);	// assume dynamics never consume full sphere
			if (ssa < r_ssaLOD_A)
				_use_lod				= TRUE;
		}

		if (_use_lod)
			add_Dynamic					(pV->m_lod, u32_max);
		else 
		{
			pV->CalculateBones			(TRUE);
			pV->CalculateWallmarks		();		//. bug?
			for (auto C : pV->children)
				add_Dynamic				(C, planes);
		}
		break;
	}
	default:
		if (pVisual)
			r_dsgraph_insert_dynamic	(pVisual, Tpos());
	}
}

void CRender::add_Static(dxRender_Visual* pVisual, u32 planes)
{
	if (planes != u32_max)
	{
		// Check frustum visibility and calculate distance to visual's center
		vis_data& vis					= pVisual->vis;
		EFC_Visible	VIS					= View->testSAABB(vis.sphere.P, vis.sphere.R, vis.box.data(), planes);
		if (VIS == fcvNone || !HOM.visible(vis))
			return;
		if (VIS == fcvFully)	//further tests unnecessary
			planes						= u32_max;
	}

	switch (pVisual->Type)
	{
	case MT_PARTICLE_GROUP:
	{
		auto pG							= reinterpret_cast<PS::CParticleGroup*>(pVisual);
		for (auto& I : pG->items)
		{
			if (I._effect)
				add_Dynamic				(I._effect, planes);
			for (auto C : I._children_related)
				add_Dynamic				(C, planes);
			for (auto C : I._children_free)
				add_Dynamic				(C, planes);
		}
		break;
	}
	case MT_HIERRARHY:
	{
		auto pV							= reinterpret_cast<FHierrarhyVisual*>(pVisual);
		for (auto C : pV->children)
			add_Static					(C, planes);
		break;
	}
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		auto pV							= reinterpret_cast<CKinematics*>(pVisual);
		pV->CalculateBones				(TRUE);
		for (auto C : pV->children)
			add_Static					(C, planes);
		break;
	}
	case MT_LOD:
	{
		auto pV							= reinterpret_cast<FLOD*>(pVisual);
		float D;
		float ssa						= CalcSSA(D, pV->vis.sphere.P, pV);
		ssa								*= pV->lod_factor;
		if (ssa < r_ssaLOD_A)	
		{
			if (ssa < r_ssaDISCARD)
				return;
			mapLOD_Node* N				= mapLOD.insertInAnyWay(D);
			N->val.ssa					= ssa;
			N->val.pVisual				= pVisual;
		}

		if (ssa > r_ssaLOD_B || phase == PHASE_SMAP)
			for (auto C : pV->children)	
				add_Static				(C, u32_max);
		break;
	}
	case MT_TREE_ST:
	case MT_TREE_PM:
	default:
		r_dsgraph_insert_static			(pVisual);
	}
}
