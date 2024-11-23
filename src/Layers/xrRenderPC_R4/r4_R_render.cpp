#include "stdafx.h"
#include "../../xrEngine/igame_persistent.h"
#include "../xrRender/FBasicVisual.h"
#include "../../xrEngine/customhud.h"
#include "../../xrEngine/xr_object.h"

#include "../xrRender/QueryHelper.h"

void CRender::render_main(Fmatrix& m_ViewProjection, bool _fportals)
{
	PIX_EVENT							(render_main);
	++marker;
	set_Object							(nullptr);

	// Calculate sector(s) and their objects
	if (!pLastSector)
	{
		if (g_pGameLevel && (phase == PHASE_NORMAL))
			g_hud->Render_Last			();		// HUD
		return;
	}

	//!!!
	//!!! BECAUSE OF PARALLEL HOM RENDERING TRY TO DELAY ACCESS TO HOM AS MUCH AS POSSIBLE
	//!!!
	{
		// Traverse object database
		g_SpatialSpace->q_frustum		(lstRenderables,
			ISpatial_DB::O_ORDERED, STYPE_RENDERABLE + STYPE_LIGHTSOURCE, Device.camera.view_base);

		// (almost) Exact sorting order (front-to-back)
		lstRenderables.sort				(
			[](auto CR$ _1, auto CR$ _2)
			{
				float d1				= _1->spatial.sphere.P.distance_to_sqr(Device.camera.position);
				float d2				= _2->spatial.sphere.P.distance_to_sqr(Device.camera.position);
				return					(d1 < d2);
			}
		);

		// Determine visibility for dynamic part of scene
		u32 uID_LTRACK					= 0xffffffff;
		if (phase==PHASE_NORMAL)
		{
			++uLastLTRACK;
			if (lstRenderables.size())
				uID_LTRACK				= uLastLTRACK%lstRenderables.size();

			// update light-vis for current entity / actor
			if (auto O = g_pGameLevel->CurrentViewEntity())
				if (auto R = reinterpret_cast<CROS_impl*>(O->ROS()))
					R->update			(O);

			// update light-vis for selected entity
			// track lighting environment
			if (lstRenderables.size())
				if (auto renderable = lstRenderables[uID_LTRACK]->dcast_Renderable())
					if (auto T = reinterpret_cast<CROS_impl*>(renderable->renderable_ROS()))
						T->update		(renderable);
		}
	}

	// Traverse sector/portal structure
	PortalTraverser.traverse(pLastSector, Device.camera.view_base, Device.camera.position, m_ViewProjection,
		CPortalTraverser::VQ_HOM + CPortalTraverser::VQ_SSA + CPortalTraverser::VQ_FADE);
	
	bool check_lense					= Device.SVP.isActive() && !Device.SVP.isRendering();
	m_planes_lense						= (check_lense) ? Device.SVP.lenseView().getMask() : u32_max;

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

	for (auto spatial : lstRenderables)
		process_spatial					(spatial);
	set_Object							(nullptr);

	if (g_pGameLevel && (phase == PHASE_NORMAL))
		g_hud->Render_Last				();		// HUD
}

void CRender::process_spatial(ISpatial* CR$ spatial)
{
	spatial->spatial_updatesector		();
	CSector* sector						= reinterpret_cast<CSector*>(spatial->spatial.sector);
	if (!sector)
		return;	// disassociated from S/P structure

	if (spatial->spatial.type & STYPE_LIGHTSOURCE)
	{
		// lightsource
		light* L						= reinterpret_cast<light*>(spatial->dcast_Light());
		R_ASSERT						(L);
		if (L->get_LOD() > EPS_L)
			if (HOM.visible(L->get_homdata()))
				Lights.add_light		(L);
		return;
	}

	if (!(spatial->spatial.type & STYPE_RENDERABLE))
		return;

	if (PortalTraverser.i_marker != sector->r_marker)
		return;	// inactive (untouched) sector

	auto R								= spatial->dcast_Renderable();
	if (ps_r__render_distance_sqr && R->getDistanceToCamera() > ps_r__render_distance_sqr)
		return;

	for (auto& view : sector->r_frustums)
	{
		if (view.testSphere_dirty(spatial->spatial.sphere.P, spatial->spatial.sphere.R))
		{
			// Occlusion
			//	casting is faster then using getVis method
			auto visual					= reinterpret_cast<dxRender_Visual*>(R->renderable.visual);
			auto& vis					= visual->vis;
			auto backup					= vis.box;
			vis.box.xform				(R->renderable.xform);
			BOOL bVisible				= HOM.visible(vis);
			vis.box						= backup;
			if (bVisible)
			{
				set_Object				(R);
				R->renderable_Render	();
			}
			return;
		}
	}
}

void CRender::render_menu()
{
	PIX_EVENT(render_menu);
	//	Globals
	RCache.set_CullMode				(CULL_CCW);
	RCache.set_Stencil				(FALSE);
	RCache.set_ColorWriteEnable		();

	// Main Render
	{
		Target->u_setrt(Target->rt_Generic_0,0,0,HW.pBaseZB);		// LDR RT
		g_pGamePersistent->OnRenderPPUI_main()	;	// PP-UI
	}

	// Distort
	{
		FLOAT ColorRGBA[4] = {127.0f/255.0f, 127.0f/255.0f, 0.0f, 127.0f/255.0f};
		Target->u_setrt(Target->rt_Generic_1,0,0,HW.pBaseZB);		// Now RT is a distortion mask
		HW.pContext->ClearRenderTargetView(Target->rt_Generic_1->pRT, ColorRGBA);		
		g_pGamePersistent->OnRenderPPUI_PP	()	;	// PP-UI
	}

	// Actual Display
	Target->u_setrt					( Device.dwWidth,Device.dwHeight,HW.pBaseRT,NULL,NULL,HW.pBaseZB);
	RCache.set_Shader				( Target->s_menu	);
	RCache.set_Geometry				( Target->g_menu	);

	Fvector2						p0,p1;
	u32								Offset;
	u32		C						= color_rgba	(255,255,255,255);
	float	_w						= float(Device.dwWidth);
	float	_h						= float(Device.dwHeight);
	float	d_Z						= EPS_S;
	float	d_W						= 1.f;
	p0.set							(.5f/_w, .5f/_h);
	p1.set							((_w+.5f)/_w, (_h+.5f)/_h );

	FVF::TL* pv						= (FVF::TL*) RCache.Vertex.Lock	(4,Target->g_menu->vb_stride,Offset);
	pv->set							(EPS,			float(_h+EPS),	d_Z,	d_W, C, p0.x, p1.y);	pv++;
	pv->set							(EPS,			EPS,			d_Z,	d_W, C, p0.x, p0.y);	pv++;
	pv->set							(float(_w+EPS),	float(_h+EPS),	d_Z,	d_W, C, p1.x, p1.y);	pv++;
	pv->set							(float(_w+EPS),	EPS,			d_Z,	d_W, C, p1.x, p0.y);	pv++;
	RCache.Vertex.Unlock			(4,Target->g_menu->vb_stride);
	RCache.Render					(D3DPT_TRIANGLELIST,Offset,0,4,0,2);
}

extern u32 g_r;
void CRender::Render		()
{
	PIX_EVENT(CRender_Render);

	g_r						= 1;
	VERIFY					(0==mapDistort.size());

	rmNormal();

	bool	_menu_pp		= g_pGamePersistent?g_pGamePersistent->OnRenderPPUI_query():false;
	if (_menu_pp)			{
		render_menu			()	;
		return					;
	};

	IMainMenu*	pMainMenu = g_pGamePersistent?g_pGamePersistent->m_pMainMenu:0;
	bool	bMenu = pMainMenu?pMainMenu->CanSkipSceneRendering():false;

	if( !(g_pGameLevel && g_hud)
		|| bMenu)	
	{
		Target->u_setrt				( Device.dwWidth,Device.dwHeight,HW.pBaseRT,NULL,NULL,HW.pBaseZB);
		return;
	}

	if( m_bFirstFrameAfterReset )
	{
		m_bFirstFrameAfterReset = false;
		return;
	}

	// Configure
	RImplementation.o.distortion		= FALSE;		// disable distorion
	bool bSUN							= !o.sunstatic && ps_r2_ls_flags.test(R2FLAG_SUN) && !Device.SVP.isRendering();
	if (bSUN)
	{
		Fcolor sun_color				= reinterpret_cast<light*>(Lights.sun_adapted._get())->color;
		if (fIsZero(u_diffuse2s(sun_color.r, sun_color.g, sun_color.b)))
			bSUN						= false;
	}

	// HOM
	View								= 0;
	HOM.Enable							();
	HOM.Render							(Device.camera.view_base);

	//******* Z-prefill calc - DEFERRER RENDERER
	if (ps_r2_ls_flags.test(R2FLAG_ZFILL))
	{
		PIX_EVENT(DEFER_Z_FILL);
		Device.Statistic->RenderCALC.Begin			();
		float		z_distance	= ps_r2_zfill		;
		Fmatrix		m_zfill, m_project				;
		m_project.build_projection	(
			deg2rad(Device.camera.fov/* *Device.camera.aspect*/), 
			Device.camera.aspect, VIEWPORT_NEAR, 
			z_distance * g_pGamePersistent->Environment().CurrentEnv->far_plane);
		m_zfill.mul	(m_project,Device.camera.view);
		r_pmask										(true,false);	// enable priority "0"
		set_Recorder								(NULL)		;
		phase										= PHASE_SMAP;
		render_main									(m_zfill,false)	;
		r_pmask										(true,false);	// disable priority "1"
		Device.Statistic->RenderCALC.End				( )			;

		// flush
		Target->phase_scene_prepare					();
		RCache.set_ColorWriteEnable					(FALSE);
		r_dsgraph_render_graph						(0);
		RCache.set_ColorWriteEnable					( );
	}
	else
		Target->phase_scene_prepare					();

	//*******
	// Sync point
	Device.Statistic->RenderDUMP_Wait_S.Begin();
	if (!Device.SVP.isActive() || Device.SVP.isRendering())
	{
		CTimer							T;
		T.Start							();
		BOOL result						= FALSE;
		while (GetData(q_sync_point[q_sync_count], &result, sizeof(result)) == S_FALSE)
		{
			if (!SwitchToThread())
				Sleep					(ps_r2_wait_sleep);

			if (T.GetElapsed_ms() > 500)
			{
				result					= FALSE;
				break;
			}
		}
		q_sync_count					= (q_sync_count + 1) % HW.Caps.iGPUNum;
		CHK_DX							(EndQuery(q_sync_point[q_sync_count]));
	}
	Device.Statistic->RenderDUMP_Wait_S.End();

	//******* Main calc - DEFERRER RENDERER
	// Main calc
	Device.Statistic->RenderCALC.Begin			();
	r_pmask										(true,false,true);	// enable priority "0",+ capture wmarks
	if (bSUN)									set_Recorder	(&main_coarse_structure);
	else										set_Recorder	(NULL);
	phase										= PHASE_NORMAL;
	render_main									(Device.camera.full_transform,true);
	set_Recorder								(NULL);
	r_pmask										(true,false);	// disable priority "1"
	Device.Statistic->RenderCALC.End			();

	BOOL	split_the_scene_to_minimize_wait		= FALSE;
	if (ps_r2_ls_flags.test(R2FLAG_EXP_SPLIT_SCENE))	split_the_scene_to_minimize_wait=TRUE;

	//******* Main render :: PART-0	-- first
	if (!split_the_scene_to_minimize_wait)
	{
		PIX_EVENT(DEFER_PART0_NO_SPLIT);
		// level, DO NOT SPLIT
		Target->phase_scene_begin				();
		r_dsgraph_render_hud					();
		r_dsgraph_render_graph					(0);
		r_dsgraph_render_lods					(true,true);
		if(Details)	Details->Render				();
		Target->phase_scene_end					();
	} 
	else 
	{
		PIX_EVENT(DEFER_PART0_SPLIT);
		// level, SPLIT
		Target->phase_scene_begin				();
		r_dsgraph_render_graph					(0);
		Target->disable_aniso					();
	}

	//******* Occlusion testing of volume-limited light-sources
	Target->phase_occq							();
	LP_normal.clear								();
	LP_pending.clear							();
   if( RImplementation.o.dx10_msaa )
      RCache.set_ZB( RImplementation.Target->rt_MSAADepth->pZRT );
	{
		PIX_EVENT(DEFER_TEST_LIGHT_VIS);
		// perform tests
		u32	count			= 0;
		light_Package&	LP	= Lights.package;

		// stats
		stats.l_shadowed	= LP.v_shadowed.size();
		stats.l_unshadowed	= LP.v_point.size() + LP.v_spot.size();
		stats.l_total		= stats.l_shadowed + stats.l_unshadowed;

		// perform tests
		count				= _max	(count,LP.v_point.size());
		count				= _max	(count,LP.v_spot.size());
		count				= _max	(count,LP.v_shadowed.size());
		for (u32 it=0; it<count; it++)	{
			if (it<LP.v_point.size())		{
				light*	L			= LP.v_point	[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_point.push_back	(L);
				else				LP_normal.v_point.push_back		(L);
			}
			if (it<LP.v_spot.size())		{
				light*	L			= LP.v_spot		[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_spot.push_back		(L);
				else				LP_normal.v_spot.push_back		(L);
			}
			if (it<LP.v_shadowed.size())	{
				light*	L			= LP.v_shadowed	[it];
				L->vis_prepare		();
				if (L->vis.pending)	LP_pending.v_shadowed.push_back	(L);
				else				LP_normal.v_shadowed.push_back	(L);
			}
		}
	}
	LP_normal.sort							();
	LP_pending.sort							();

   //******* Main render :: PART-1 (second)
	if (split_the_scene_to_minimize_wait)	
	{
		PIX_EVENT(DEFER_PART1_SPLIT);
		// skybox can be drawn here
		if (0)
		{

			if( !RImplementation.o.dx10_msaa )
				Target->u_setrt		( Target->rt_Generic_0,	Target->rt_Generic_1,0,HW.pBaseZB );
			else
				Target->u_setrt		( Target->rt_Generic_0_r,	Target->rt_Generic_1,0,RImplementation.Target->rt_MSAADepth->pZRT );
			RCache.set_CullMode	( CULL_NONE );
			RCache.set_Stencil	( FALSE		);

			// draw skybox
			RCache.set_ColorWriteEnable					();
			//CHK_DX(HW.pDevice->SetRenderState			( D3DRS_ZENABLE,	FALSE				));
			RCache.set_Z(FALSE);
			g_pGamePersistent->Environment().RenderSky	();
			//CHK_DX(HW.pDevice->SetRenderState			( D3DRS_ZENABLE,	TRUE				));
			RCache.set_Z(TRUE);
		}

		// level
		Target->phase_scene_begin				();
		r_dsgraph_render_hud					();
		r_dsgraph_render_lods					(true,true);
		if(Details)	Details->Render				();
		Target->phase_scene_end					();
	}

	if (g_hud && g_hud->RenderActiveItemUIQuery())
	{
		Target->phase_wallmarks();
		r_dsgraph_render_hud_ui();
	}

	// Wall marks
	if(Wallmarks)	
	{
		PIX_EVENT(DEFER_WALLMARKS);
		Target->phase_wallmarks					();
		g_r										= 0;
		Wallmarks->Render						();				// wallmarks has priority as normal geometry
	}

	// Update incremental shadowmap-visibility solver
	{
		PIX_EVENT(DEFER_FLUSH_OCCLUSION);
		u32 it=0;
		for (it=0; it<Lights_LastFrame.size(); it++)	{
			if (0==Lights_LastFrame[it])	continue	;
			try {
				Lights_LastFrame[it]->svis.flushoccq()	;
			} catch (...)
			{
				Msg	("! Failed to flush-OCCq on light [%d] %X",it,*(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear	();
	}

   // full screen pass to mark msaa-edge pixels in highest stencil bit
   if( RImplementation.o.dx10_msaa )
   {
	   PIX_EVENT( MARK_MSAA_EDGES );
      Target->mark_msaa_edges();
   }

	//	TODO: DX10: Implement DX10 rain.
	if (ps_r2_ls_flags.test(R3FLAG_DYN_WET_SURF))
	{
		PIX_EVENT(DEFER_RAIN);
		render_rain();
	}

	// Directional light - fucking sun
	if (bSUN)	
	{
		PIX_EVENT(DEFER_SUN);
		RImplementation.stats.l_visible		++;
		if( !ps_r2_ls_flags_ext.is(R2FLAGEXT_SUN_OLD))
			render_sun_cascades					();
		else
		{
			render_sun_near						();
			render_sun							();
			render_sun_filtered					();
		}
		Target->accum_direct_blend			();
	}

	{
		PIX_EVENT(DEFER_SELF_ILLUM);
		Target->phase_accumulator			();
		// Render emissive geometry, stencil - write 0x0 at pixel pos
		RCache.set_xform_project			(Device.camera.project); 
		RCache.set_xform_view				(Device.camera.view);
		// Stencil - write 0x1 at pixel pos - 
      if( !RImplementation.o.dx10_msaa )
		   RCache.set_Stencil					( TRUE,D3DCMP_ALWAYS,0x01,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
      else
		   RCache.set_Stencil					( TRUE,D3DCMP_ALWAYS,0x01,0xff,0x7f,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		//RCache.set_Stencil				(TRUE,D3DCMP_ALWAYS,0x00,0xff,0xff,D3DSTENCILOP_KEEP,D3DSTENCILOP_REPLACE,D3DSTENCILOP_KEEP);
		RCache.set_CullMode					(CULL_CCW);
		RCache.set_ColorWriteEnable			();
		RImplementation.r_dsgraph_render_emissive();
	}

	// Lighting, non dependant on OCCQ
	{
		PIX_EVENT(DEFER_LIGHT_NO_OCCQ);
		Target->phase_accumulator				();
		HOM.Disable								();
		render_lights							(LP_normal);
	}

	// Lighting, dependant on OCCQ
	{
		PIX_EVENT(DEFER_LIGHT_OCCQ);
		render_lights							(LP_pending);
	}

	// Postprocess
	{
		PIX_EVENT(DEFER_LIGHT_COMBINE);
		Target->phase_combine					();
	}

	VERIFY	(0==mapDistort.size());
}

void CRender::render_forward				()
{
	VERIFY	(0==mapDistort.size());
	RImplementation.o.distortion				= RImplementation.o.distortion_enabled;	// enable distorion

	//******* Main render - second order geometry (the one, that doesn't support deffering)
	//.todo: should be done inside "combine" with estimation of of luminance, tone-mapping, etc.
	{
		// level
		r_pmask									(false,true);			// enable priority "1"
		phase									= PHASE_NORMAL;
		render_main								(Device.camera.full_transform,false);//
		//	Igor: we don't want to render old lods on next frame.
		mapLOD.clear							();
		r_dsgraph_render_graph					(1)	;					// normal level, secondary priority
		PortalTraverser.fade_render				()	;					// faded-portals
		r_dsgraph_render_sorted					()	;					// strict-sorted geoms
		g_pGamePersistent->Environment().RenderLast()	;					// rain/thunder-bolts
	}

	RImplementation.o.distortion				= FALSE;				// disable distorion
}

void CRender::RenderToTarget()
{
	ID3DResource* res;
	HW.pBaseRT->GetResource(&res);
	HW.pContext->CopyResource(Target->rt_secondVP->pSurface, res);
}
