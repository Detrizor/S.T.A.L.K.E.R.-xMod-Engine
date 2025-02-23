#include "stdafx.h"

#include "../../xrEngine/render.h"
#include "../../xrEngine/irenderable.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"
#include "../../xrEngine/CustomHUD.h"

#include "FBasicVisual.h"

using namespace		R_dsgraph;

extern float		r_ssaDISCARD;
extern float		r_ssaDONTSORT;
extern float		r_ssaHZBvsTEX;
extern float		r_ssaGLOD_start,	r_ssaGLOD_end;

ICF float calcLOD	(float ssa/*fDistSq*/, float R)
{
	return			_sqrt(clampr((ssa - r_ssaGLOD_end)/(r_ssaGLOD_start-r_ssaGLOD_end),0.f,1.f));
}

// NORMAL
IC	bool	cmp_normal_items		(const _NormalItem& N1, const _NormalItem& N2)
{	return (N1.ssa > N2.ssa);		}

void __fastcall mapNormal_Render	(mapNormalItems& N)
{
	// *** DIRECT ***
	N.sort					(cmp_normal_items);
	_NormalItem				*I=&*N.begin(), *E = &*N.end();
	for (; I!=E; I++)		{
		_NormalItem&		Ni	= *I;
		float LOD = calcLOD(Ni.ssa,Ni.pVisual->vis.sphere.R);
		RCache.LOD.set_LOD(LOD);
		Ni.pVisual->Render	(LOD);
	}
}

// Matrix
IC	bool	cmp_matrix_items		(const _MatrixItem& N1, const _MatrixItem& N2)
{	return (N1.ssa > N2.ssa);		}

void __fastcall mapMatrix_Render	(mapMatrixItems& N)
{
	// *** DIRECT ***
	N.sort					(cmp_matrix_items);
	_MatrixItem				*I=&*N.begin(), *E = &*N.end();
	for (; I!=E; I++)		{
		_MatrixItem&	Ni				= *I;
		RCache.set_xform_world			(Ni.Matrix);
		RImplementation.apply_object	(Ni.pObject);
		RImplementation.apply_lmaterial	();

		float LOD = calcLOD(Ni.ssa,Ni.pVisual->vis.sphere.R);
		RCache.LOD.set_LOD(LOD);
		Ni.pVisual->Render(LOD);
	}
	N.clear	();
}

// ALPHA
void __fastcall sorted_L1		(mapSorted_Node *N)
{
	VERIFY (N);
	dxRender_Visual *V				= N->val.pVisual;
	VERIFY (V && V->shader._get());
	RCache.set_Element				(N->val.se);
	RCache.set_xform_world			(N->val.Matrix);
	RImplementation.apply_object	(N->val.pObject);
	RImplementation.apply_lmaterial	();
	V->Render						(calcLOD(N->key,V->vis.sphere.R));
}

IC	bool	cmp_vs_nrm			(mapNormalVS::TNode* N1, mapNormalVS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}
IC	bool	cmp_vs_mat			(mapMatrixVS::TNode* N1, mapMatrixVS::TNode* N2)
{
	return (N1->val.ssa > N2->val.ssa);
}

IC	bool	cmp_ps_nrm			(mapNormalPS::TNode* N1, mapNormalPS::TNode* N2)
{
	return (N1->val.mapCS.ssa > N2->val.mapCS.ssa);
}
IC	bool	cmp_ps_mat			(mapMatrixPS::TNode* N1, mapMatrixPS::TNode* N2)
{
	return (N1->val.mapCS.ssa > N2->val.mapCS.ssa);
}

IC	bool	cmp_gs_nrm			(mapNormalGS::TNode* N1, mapNormalGS::TNode* N2)			{	return (N1->val.ssa > N2->val.ssa);		}
IC	bool	cmp_gs_mat			(mapMatrixGS::TNode* N1, mapMatrixGS::TNode* N2)			{	return (N1->val.ssa > N2->val.ssa);		}

IC	bool	cmp_cs_nrm			(mapNormalCS::TNode* N1, mapNormalCS::TNode* N2)			{	return (N1->val.ssa > N2->val.ssa);		}
IC	bool	cmp_cs_mat			(mapMatrixCS::TNode* N1, mapMatrixCS::TNode* N2)			{	return (N1->val.ssa > N2->val.ssa);		}

IC	bool	cmp_states_nrm		(mapNormalStates::TNode* N1, mapNormalStates::TNode* N2)	{	return (N1->val.ssa > N2->val.ssa);		}
IC	bool	cmp_states_mat		(mapMatrixStates::TNode* N1, mapMatrixStates::TNode* N2)	{	return (N1->val.ssa > N2->val.ssa);		}

IC	bool	cmp_textures_lex2_nrm	(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	if ((*t1)[0] < (*t2)[0])	return true;
	if ((*t1)[0] > (*t2)[0])	return false;
	if ((*t1)[1] < (*t2)[1])	return true;
	else						return false;
}
IC	bool	cmp_textures_lex2_mat	(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	if ((*t1)[0] < (*t2)[0])	return true;
	if ((*t1)[0] > (*t2)[0])	return false;
	if ((*t1)[1] < (*t2)[1])	return true;
	else						return false;
}
IC	bool	cmp_textures_lex3_nrm	(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	if ((*t1)[0] < (*t2)[0])	return true;
	if ((*t1)[0] > (*t2)[0])	return false;
	if ((*t1)[1] < (*t2)[1])	return true;
	if ((*t1)[1] > (*t2)[1])	return false;
	if ((*t1)[2] < (*t2)[2])	return true;
	else						return false;
}
IC	bool	cmp_textures_lex3_mat	(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	if ((*t1)[0] < (*t2)[0])	return true;
	if ((*t1)[0] > (*t2)[0])	return false;
	if ((*t1)[1] < (*t2)[1])	return true;
	if ((*t1)[1] > (*t2)[1])	return false;
	if ((*t1)[2] < (*t2)[2])	return true;
	else						return false;
}
IC	bool	cmp_textures_lexN_nrm	(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	return std::lexicographical_compare(t1->begin(),t1->end(),t2->begin(),t2->end());
}
IC	bool	cmp_textures_lexN_mat	(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2){	
	STextureList*	t1			= N1->key;
	STextureList*	t2			= N2->key;
	return std::lexicographical_compare(t1->begin(),t1->end(),t2->begin(),t2->end());
}
IC	bool	cmp_textures_ssa_nrm	(mapNormalTextures::TNode* N1, mapNormalTextures::TNode* N2){	
	return (N1->val.ssa > N2->val.ssa);		
}
IC	bool	cmp_textures_ssa_mat	(mapMatrixTextures::TNode* N1, mapMatrixTextures::TNode* N2){	
	return (N1->val.ssa > N2->val.ssa);		
}

void		sort_tlist_nrm			
(
 xr_vector<mapNormalTextures::TNode*,render_alloc<mapNormalTextures::TNode*> >& lst, 
 xr_vector<mapNormalTextures::TNode*,render_alloc<mapNormalTextures::TNode*> >& temp, 
 mapNormalTextures&					textures, 
 BOOL	bSSA
 )
{
	int amount			= textures.begin()->key->size();
	if (bSSA)	
	{
		if (amount<=1)
		{
			// Just sort by SSA
			textures.getANY_P			(lst);
			lst.sort					(cmp_textures_ssa_nrm);
		} 
		else 
		{
			// Split into 2 parts
			mapNormalTextures::TNode* _it	= textures.begin	();
			mapNormalTextures::TNode* _end	= textures.end		();
			for (; _it!=_end; _it++)	{
				if (_it->val.ssa > r_ssaHZBvsTEX)	lst.push_back	(_it);
				else								temp.push_back	(_it);
			}

			// 1st - part - SSA, 2nd - lexicographically
			lst.sort					(cmp_textures_ssa_nrm);
			if (2==amount)
				temp.sort				(cmp_textures_lex2_nrm);
			else if (3==amount)
				temp.sort				(cmp_textures_lex3_nrm);
			else
				temp.sort				(cmp_textures_lexN_nrm);

			// merge lists
			lst.insert					(lst.end(),temp.begin(),temp.end());
		}
	}
	else 
	{
		textures.getANY_P			(lst);
		if (2==amount)
			lst.sort				(cmp_textures_lex2_nrm);
		else if (3==amount)
			lst.sort				(cmp_textures_lex3_nrm);
		else
			lst.sort				(cmp_textures_lexN_nrm);
	}
}

void		sort_tlist_mat			
(
 xr_vector<mapMatrixTextures::TNode*,render_alloc<mapMatrixTextures::TNode*> >& lst,
 xr_vector<mapMatrixTextures::TNode*,render_alloc<mapMatrixTextures::TNode*> >& temp,
 mapMatrixTextures&					textures,
 BOOL	bSSA
 )
{
	int amount			= textures.begin()->key->size();
	if (bSSA)	
	{
		if (amount<=1)
		{
			// Just sort by SSA
			textures.getANY_P			(lst);
			lst.sort					(cmp_textures_ssa_mat);
		} 
		else 
		{
			// Split into 2 parts
			mapMatrixTextures::TNode* _it	= textures.begin	();
			mapMatrixTextures::TNode* _end	= textures.end		();
			for (; _it!=_end; _it++)	{
				if (_it->val.ssa > r_ssaHZBvsTEX)	lst.push_back	(_it);
				else								temp.push_back	(_it);
			}

			// 1st - part - SSA, 2nd - lexicographically
			lst.sort					(cmp_textures_ssa_mat);
			if (2==amount)
				temp.sort				(cmp_textures_lex2_mat);
			else if (3==amount)
				temp.sort				(cmp_textures_lex3_mat);
			else
				temp.sort				(cmp_textures_lexN_mat);

			// merge lists
			lst.insert					(lst.end(),temp.begin(),temp.end());
		}
	}
	else 
	{
		textures.getANY_P			(lst);
		if (2==amount)
			lst.sort				(cmp_textures_lex2_mat);
		else if (3==amount)
			lst.sort				(cmp_textures_lex3_mat);
		else
			lst.sort				(cmp_textures_lexN_mat);
	}
}

void R_dsgraph_structure::r_dsgraph_render_graph	(u32	_priority, bool _clear)
{

	//PIX_EVENT(r_dsgraph_render_graph);
	Device.Statistic->RenderDUMP.Begin		();

	// **************************************************** NORMAL
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	{
		RCache.set_xform_world			(Fidentity);

		// Render several passes
		for ( u32 iPass = 0; iPass<SHADER_PASSES_MAX; ++iPass)
		{
			//mapNormalVS&	vs				= mapNormal	[_priority];
			mapNormalVS&	vs				= mapNormalPasses[_priority][iPass];
			vs.getANY_P						(nrmVS);
			nrmVS.sort						(cmp_vs_nrm);
			for (u32 vs_id=0; vs_id<nrmVS.size(); vs_id++)
			{
				mapNormalVS::TNode*	Nvs			= nrmVS[vs_id];
				RCache.set_VS					(Nvs->key);

				//	GS setup
				mapNormalGS&		gs			= Nvs->val;		gs.ssa	= 0;

				gs.getANY_P						(nrmGS);
				nrmGS.sort						(cmp_gs_nrm);
				for (u32 gs_id=0; gs_id<nrmGS.size(); gs_id++)
				{
					mapNormalGS::TNode*	Ngs			= nrmGS[gs_id];
					RCache.set_GS					(Ngs->key);	

					mapNormalPS&		ps			= Ngs->val;		ps.ssa	= 0;

					ps.getANY_P						(nrmPS);
					nrmPS.sort						(cmp_ps_nrm);
					for (u32 ps_id=0; ps_id<nrmPS.size(); ps_id++)
					{
						mapNormalPS::TNode*	Nps			= nrmPS[ps_id];
						RCache.set_PS					(Nps->key);	
						mapNormalCS&		cs			= Nps->val.mapCS;		cs.ssa	= 0;
						RCache.set_HS(Nps->val.hs);
						RCache.set_DS(Nps->val.ds);
						cs.getANY_P						(nrmCS);
						nrmCS.sort						(cmp_cs_nrm);
						for (u32 cs_id=0; cs_id<nrmCS.size(); cs_id++)
						{
							mapNormalCS::TNode*	Ncs			= nrmCS[cs_id];
							RCache.set_Constants			(Ncs->key);

							mapNormalStates&	states		= Ncs->val;		states.ssa	= 0;
							states.getANY_P					(nrmStates);
							nrmStates.sort					(cmp_states_nrm);
							for (u32 state_id=0; state_id<nrmStates.size(); state_id++)
							{
								mapNormalStates::TNode*	Nstate		= nrmStates[state_id];
								RCache.set_States					(Nstate->key);

								mapNormalTextures&		tex			= Nstate->val;	tex.ssa =	0;
								sort_tlist_nrm						(nrmTextures,nrmTexturesTemp,tex,true);
								for (u32 tex_id=0; tex_id<nrmTextures.size(); tex_id++)
								{
									mapNormalTextures::TNode*	Ntex	= nrmTextures[tex_id];
									RCache.set_Textures					(Ntex->key);
									RImplementation.apply_lmaterial		();

									mapNormalItems&				items	= Ntex->val;		items.ssa	= 0;
									mapNormal_Render					(items);
									if (_clear)				items.clear	();
								}
								nrmTextures.clear		();
								nrmTexturesTemp.clear	();
								if(_clear) tex.clear	();
							}
							nrmStates.clear			();
							if(_clear) states.clear	();
						}
						nrmCS.clear				();
						if(_clear) cs.clear		();

					}
					nrmPS.clear				();
					if(_clear) ps.clear		();
				}
				nrmGS.clear				();
				if(_clear) gs.clear		();
			}
			nrmVS.clear				();
			if(_clear) vs.clear		();
		}
	}

	// **************************************************** MATRIX
	// Perform sorting based on ScreenSpaceArea
	// Sorting by SSA and changes minimizations
	// Render several passes
	for ( u32 iPass = 0; iPass<SHADER_PASSES_MAX; ++iPass)
	{
		//mapMatrixVS&	vs				= mapMatrix	[_priority];
		mapMatrixVS&	vs				= mapMatrixPasses[_priority][iPass];
		vs.getANY_P						(matVS);
		matVS.sort						(cmp_vs_mat);
		for (u32 vs_id=0; vs_id<matVS.size(); vs_id++)	{
			mapMatrixVS::TNode*	Nvs			= matVS[vs_id];
			RCache.set_VS					(Nvs->key);	

			mapMatrixGS&		gs			= Nvs->val;		gs.ssa	= 0;

			gs.getANY_P						(matGS);
			matGS.sort						(cmp_gs_mat);
			for (u32 gs_id=0; gs_id<matGS.size(); gs_id++)
			{
				mapMatrixGS::TNode*	Ngs			= matGS[gs_id];
				RCache.set_GS					(Ngs->key);	

				mapMatrixPS&		ps			= Ngs->val;		ps.ssa	= 0;

				ps.getANY_P						(matPS);
				matPS.sort						(cmp_ps_mat);
				for (u32 ps_id=0; ps_id<matPS.size(); ps_id++)
				{
					mapMatrixPS::TNode*	Nps			= matPS[ps_id];
					RCache.set_PS					(Nps->key);	

					mapMatrixCS&		cs			= Nps->val.mapCS;		cs.ssa	= 0;
					RCache.set_HS(Nps->val.hs);
					RCache.set_DS(Nps->val.ds);
					cs.getANY_P						(matCS);
					matCS.sort						(cmp_cs_mat);
					for (u32 cs_id=0; cs_id<matCS.size(); cs_id++)
					{
						mapMatrixCS::TNode*	Ncs			= matCS[cs_id];
						RCache.set_Constants			(Ncs->key);

						mapMatrixStates&	states		= Ncs->val;		states.ssa	= 0;
						states.getANY_P					(matStates);
						matStates.sort					(cmp_states_mat);
						for (u32 state_id=0; state_id<matStates.size(); state_id++)
						{
							mapMatrixStates::TNode*	Nstate		= matStates[state_id];
							RCache.set_States					(Nstate->key);

							mapMatrixTextures&		tex			= Nstate->val;	tex.ssa =	0;
							sort_tlist_mat						(matTextures,matTexturesTemp,tex,true);
							for (u32 tex_id=0; tex_id<matTextures.size(); tex_id++)
							{
								mapMatrixTextures::TNode*	Ntex	= matTextures[tex_id];
								RCache.set_Textures					(Ntex->key);
								RImplementation.apply_lmaterial		();

								mapMatrixItems&				items	= Ntex->val;		items.ssa	= 0;
								mapMatrix_Render					(items);
							}
							matTextures.clear		();
							matTexturesTemp.clear	();
							if(_clear) tex.clear	();
						}
						matStates.clear			();
						if(_clear) states.clear	();
					}
					matCS.clear				();
					if(_clear) cs.clear		();
				}
				matPS.clear				();
				if(_clear) ps.clear		();
			}
			matGS.clear				();
			if(_clear) gs.clear		();
		}
		matVS.clear				();
		if(_clear) vs.clear		();
	}

	Device.Statistic->RenderDUMP.End	();
}

//////////////////////////////////////////////////////////////////////////
// HUD render
extern ENGINE_API float VIEWPORT_NEAR_HUD;
void R_dsgraph_structure::r_dsgraph_render_hud	()
{
	//PIX_EVENT(r_dsgraph_render_hud);

	// Change projection
	Fmatrix Pold				= Device.camera.project;
	Fmatrix FTold				= Device.camera.full_transform;
	Fmatrix FVold				= Device.camera.view;
	Device.camera.project.build_projection(
		deg2rad(Device.camera.hud_fov),
		Device.camera.aspect, VIEWPORT_NEAR_HUD,
		g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.camera.full_transform.mul	(Device.camera.project, Device.camera.view);
	RCache.set_xform_project	(Device.camera.project);

	// Rendering
	rmNear						();
	mapHUD.traverseLR			(sorted_L1);
	mapHUD.clear				();

	rmNormal					();

	// Restore projection
	Device.camera.project				= Pold;
	Device.camera.full_transform		= FTold;
	Device.camera.view				= FVold;
	RCache.set_xform_project	(Device.camera.project);
}

void R_dsgraph_structure::r_dsgraph_render_hud_ui()
{
	VERIFY(g_hud && g_hud->RenderActiveItemUIQuery());

	// Change projection
	Fmatrix Pold				= Device.camera.project;
	Fmatrix FTold				= Device.camera.full_transform;
	Device.camera.project.build_projection(
		deg2rad(Device.camera.hud_fov),
		Device.camera.aspect, VIEWPORT_NEAR_HUD,
		g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.camera.full_transform.mul	(Device.camera.project, Device.camera.view);
	RCache.set_xform_project	(Device.camera.project);

	// Targets, use accumulator for temporary storage
	const ref_rt	rt_null;
	RCache.set_RT(0,	1);
	RCache.set_RT(0,	2);
	if( !RImplementation.o.dx10_msaa )
	{
		if (RImplementation.o.albedo_wo)	RImplementation.Target->u_setrt		(RImplementation.Target->rt_Accumulator,	rt_null,	rt_null,	HW.pBaseZB);
		else								RImplementation.Target->u_setrt		(RImplementation.Target->rt_Color,			rt_null,	rt_null,	HW.pBaseZB);
	}
	else
	{
		if (RImplementation.o.albedo_wo)	RImplementation.Target->u_setrt		(RImplementation.Target->rt_Accumulator,	rt_null,	rt_null,	RImplementation.Target->rt_MSAADepth->pZRT);
		else								RImplementation.Target->u_setrt		(RImplementation.Target->rt_Color,			rt_null,	rt_null,	RImplementation.Target->rt_MSAADepth->pZRT);
	}

	rmNear						();
	g_hud->RenderActiveItemUI	();
	rmNormal					();

	// Restore projection
	Device.camera.project				= Pold;
	Device.camera.full_transform		= FTold;
	RCache.set_xform_project	(Device.camera.project);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void	R_dsgraph_structure::r_dsgraph_render_sorted	()
{
	// Sorted (back to front)
	mapSorted.traverseRL	(sorted_L1);
	mapSorted.clear			();

	// Change projection
	Fmatrix Pold = Device.camera.project;
	Fmatrix FTold = Device.camera.full_transform;
	Device.camera.project.build_projection(
		deg2rad(Device.camera.hud_fov),
		Device.camera.aspect, VIEWPORT_NEAR_HUD,
		g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.camera.full_transform.mul(Device.camera.project, Device.camera.view);
	RCache.set_xform_project(Device.camera.project);

	// Rendering
	rmNear();
	mapHUDSorted.traverseRL(sorted_L1);
	mapHUDSorted.clear();
	rmNormal();

	// Restore projection
	Device.camera.project = Pold;
	Device.camera.full_transform = FTold;
	RCache.set_xform_project(Device.camera.project);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void	R_dsgraph_structure::r_dsgraph_render_emissive	()
{
	// Sorted (back to front)
	mapEmissive.traverseLR	(sorted_L1);
	mapEmissive.clear		();

	//	HACK: Calculate this only once

	// Change projection
	Fmatrix Pold				= Device.camera.project;
	Fmatrix FTold				= Device.camera.full_transform;
	Device.camera.project.build_projection(
		deg2rad(Device.camera.hud_fov),
		Device.camera.aspect, VIEWPORT_NEAR_HUD, 
		g_pGamePersistent->Environment().CurrentEnv->far_plane);

	Device.camera.full_transform.mul	(Device.camera.project, Device.camera.view);
	RCache.set_xform_project	(Device.camera.project);

	// Rendering
	rmNear						();
	// Sorted (back to front)
	mapHUDEmissive.traverseLR	(sorted_L1);
	mapHUDEmissive.clear		();

	rmNormal					();

	// Restore projection
	Device.camera.project				= Pold;
	Device.camera.full_transform		= FTold;
	RCache.set_xform_project	(Device.camera.project);
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void	R_dsgraph_structure::r_dsgraph_render_wmarks	()
{
	// Sorted (back to front)
	mapWmark.traverseLR	(sorted_L1);
	mapWmark.clear		();
}

//////////////////////////////////////////////////////////////////////////
// strict-sorted render
void	R_dsgraph_structure::r_dsgraph_render_distort	()
{
	// Sorted (back to front)
	mapDistort.traverseRL	(sorted_L1);
	mapDistort.clear		();
}

//////////////////////////////////////////////////////////////////////////
// sub-space rendering - shortcut to render with frustum extracted from matrix
void	R_dsgraph_structure::r_dsgraph_render_subspace	(IRender_Sector* _sector, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
	CFrustum	temp;
	temp.CreateFromMatrix			(mCombined,	FRUSTUM_P_ALL &(~FRUSTUM_P_NEAR));
	r_dsgraph_render_subspace		(_sector,&temp,mCombined,_cop,_dynamic,_precise_portals);
}

// sub-space rendering - main procedure
void	R_dsgraph_structure::r_dsgraph_render_subspace	(IRender_Sector* _sector, CFrustum* _frustum, Fmatrix& mCombined, Fvector& _cop, BOOL _dynamic, BOOL _precise_portals)
{
	VERIFY								(_sector);
	++RImplementation.marker;			// !!! critical here

	if (_precise_portals && RImplementation.rmPortals)
	{
		// Check if camera is too near to some portal - if so force DualRender
		Fvector							box_radius;
		box_radius.set					(EPS_L * 20.f, EPS_L * 20.f, EPS_L * 20.f);
		RImplementation.Sectors_xrc.box_options(CDB::OPT_FULL_TEST);
		RImplementation.Sectors_xrc.box_query(RImplementation.rmPortals, _cop, box_radius);
		for (int K = 0; K < RImplementation.Sectors_xrc.r_count(); ++K)
		{
			auto id						= RImplementation.Sectors_xrc.r_begin()[K].id;
			auto dummy					= RImplementation.rmPortals->get_tris()[id].dummy;
			auto portal					= reinterpret_cast<CPortal*>(RImplementation.Portals[dummy]);
			portal->bDualRender			= TRUE;
		}
	}

	// Traverse sector/portal structure
	PortalTraverser.traverse			(_sector, *_frustum, _cop, mCombined, 0);

	// Determine visibility for static geometry hierrarhy
	for (auto& i_sector : PortalTraverser.r_sectors)
	{
		auto sector						= reinterpret_cast<CSector*>(i_sector);
		dxRender_Visual* root			= sector->root();
		for (auto& frustum : sector->r_frustums)
		{
			set_Frustum					(&frustum);
			add_Geometry				(root);
		}
	}

	if (!_dynamic)
		return;

	set_Object							(nullptr);

	// Traverse object database
	g_SpatialSpace->q_frustum			(lstRenderables, ISpatial_DB::O_ORDERED, STYPE_RENDERABLE, *_frustum);

	auto process_spatial = [this](ISpatial* spatial)
	{
		CSector* sector					= reinterpret_cast<CSector*>(spatial->spatial.sector);
		if (!sector)
			return;	// disassociated from S/P structure

		if (PortalTraverser.i_marker != sector->r_marker)
			return;	// inactive (untouched) sector

		auto R							= spatial->dcast_Renderable();
		if (!R)
			return;

		if (ps_r__render_distance_sqr && R->getDistanceToCamera() > ps_r__render_distance_sqr)
			return;

		for (auto& view : sector->r_frustums)
		{
			if (view.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
			{
				R->renderable_Render	();
				return;
			}
		}
	};

	// Determine visibility for dynamic part of scene
	for (auto spatial : lstRenderables)
		process_spatial					(spatial);

	if (g_pGameLevel && (phase == RImplementation.PHASE_SMAP) && ps_actor_shadow_flags.test(RFLAG_ACTOR_SHADOW))
		if (auto s = g_hud->Render_Actor_Shadow())
			process_spatial				(s);
}

#include "fhierrarhyvisual.h"
#include "SkeletonCustom.h"
#include "../../xrEngine/fmesh.h"
#include "flod.h"

void	R_dsgraph_structure::r_dsgraph_render_R1_box	(IRender_Sector* _S, Fbox& BB, int sh)
{
	CSector*	S			= (CSector*)_S;
	lstVisuals.clear		();
	lstVisuals.push_back	(S->root());
	
	for (u32 test=0; test<lstVisuals.size(); test++)
	{
		dxRender_Visual*	V		= 	lstVisuals[test];
		
		// Visual is 100% visible - simply add it
		xr_vector<dxRender_Visual*>::iterator I,E;	// it may be usefull for 'hierrarhy' visuals
		
		switch (V->Type) {
		case MT_HIERRARHY:
			{
				// Add all children
				FHierrarhyVisual* pV = (FHierrarhyVisual*)V;
				I = pV->children.begin	();
				E = pV->children.end		();
				for (; I != E; ++I)		{
					dxRender_Visual* T			= *I;
					if (BB.intersect(T->vis.box))	lstVisuals.push_back(T);
				}
			}
			break;
		case MT_SKELETON_ANIM:
		case MT_SKELETON_RIGID:
			{
				// Add all children	(s)
				CKinematics * pV		= (CKinematics*)V;
				pV->CalculateBones		(TRUE);
				I = pV->children.begin	();
				E = pV->children.end		();
				for (; I != E; ++I)		{
					dxRender_Visual* T				= *I;
					if (BB.intersect(T->vis.box))	lstVisuals.push_back(T);
				}
			}
			break;
		case MT_LOD:
			{
				FLOD		* pV		=	(FLOD*) V;
				I = pV->children.begin		();
				E = pV->children.end		();
				for (; I != E; ++I)		{
					dxRender_Visual* T				= *I;
					if (BB.intersect(T->vis.box))	lstVisuals.push_back(T);
				}
			}
			break;
		default:
			{
				// Renderable visual
				ShaderElement* E	= V->shader->E[sh]._get();
				if (E && !(E->flags.bDistort))
				{
					for (u32 pass=0; pass<E->passes.size(); pass++)
					{
						RCache.set_Element			(E,pass);
						V->Render					(-1.f);
					}
				}
			}
			break;
		}
	}
}
