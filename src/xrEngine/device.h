#pragma once

#include "pure.h"
#include "stats.h"

#include "../xrcore/ftimer.h"
#include "../xrCDB/frustum.h"

extern ENGINE_API float VIEWPORT_NEAR;

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/RenderDeviceRender.h"

#ifdef INGAME_EDITOR
# include "../Include/editor/interfaces.hpp"
#endif // #ifdef INGAME_EDITOR

class engine_impl;

#pragma pack(push,4)

class IRenderDevice
{
public:
	virtual CStatsPhysics* _BCL StatPhysics() = 0;
	virtual void _BCL AddSeqFrame(pureFrame* f, bool mt) = 0;
	virtual void _BCL RemoveSeqFrame(pureFrame* f) = 0;

	virtual bool _BCL isActiveMain() const = 0;
	virtual bool _BCL isGameProcess() const = 0;
};

class ENGINE_API CRenderDeviceBase : public IRenderDevice
{
public:
	u32 dwWidth;
	u32 dwHeight;

	u32 dwPrecacheFrame;
	BOOL b_is_Ready;
	BOOL b_is_Active;

public:
	// Engine flow-control
	u32 dwFrame;

	float fTimeDelta;
	float fTimeGlobal;
	u32 dwTimeDelta;
	u32 dwTimeGlobal;
	u32 dwTimeContinual;

protected:
	u32 Timer_MM_Delta;
	CTimer_paused Timer;
	CTimer_paused TimerGlobal;

public:
	// Registrators
	CRegistrator <pureRender > seqRender;
	CRegistrator <pureAppActivate > seqAppActivate;
	CRegistrator <pureAppDeactivate > seqAppDeactivate;
	CRegistrator <pureAppStart > seqAppStart;
	CRegistrator <pureAppEnd > seqAppEnd;
	CRegistrator <pureFrame > seqFrame;
	CRegistrator <pureScreenResolutionChanged> seqResolutionChanged;

	HWND m_hWnd;

public:
	struct SCameraInfo
	{
		Fvector							position								= vZero;
		Fvector							direction								= vForward;
		Fvector							top										= vTop;
		Fvector							right									= vRight;

		Fmatrix							view									= Fidentity;
		Fmatrix							project									= Fidentity;
		Fmatrix							full_transform							= Fidentity;
		Fmatrix							full_transform_inv						= Fidentity;

		float							fov										= 75.f;
		float							hud_fov									= 75.f;
		float							aspect									= 1.f;
		float							izoom_sqr								= 1.f;

		CFrustum						view_base								= {};
	};

public:
	SCameraInfo							camera;
	Fvector								cam_position_saved;
	Fmatrix								cam_full_transform_saved;
};

#pragma pack(pop)
class ENGINE_API CRenderDevice : public CRenderDeviceBase
{
private:
	// Main objects used for creating and rendering the 3D scene
	u32 m_dwWindowStyle;
	RECT m_rcWindowBounds;
	RECT m_rcWindowClient;

	CTimer TimerMM;

	void _Create(LPCSTR shName);
	void _Destroy(BOOL bKeepTextures);
	void _SetupStates();

public:
	LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);
	u32 dwPrecacheTotal;
	float fWidth_2, fHeight_2;
	void OnWM_Activate(WPARAM wParam, LPARAM lParam);

public:

	IRenderDeviceRender* m_pRender;

	BOOL m_bNearer;
	void SetNearer(BOOL enabled)
	{
		if (enabled&&!m_bNearer)
		{
			m_bNearer = TRUE;
			camera.project._43 -= EPS_L;
		}
		else if (!enabled&&m_bNearer)
		{
			m_bNearer = FALSE;
			camera.project._43 += EPS_L;
		}
		m_pRender->SetCacheXform(camera.view, camera.project);
	}

	void DumpResourcesMemoryUsage() { m_pRender->ResourcesDumpMemoryUsage(); }

public:
	CRegistrator <pureFrame > seqFrameMT;
	CRegistrator <pureDeviceReset > seqDeviceReset;
	xr_vector <fastdelegate::FastDelegate0<> > seqParallel;

	CStats* Statistic;

	CRenderDevice()
		:
		m_pRender(0)
#ifdef INGAME_EDITOR
		, m_editor_module(0),
		m_editor_initialize(0),
		m_editor_finalize(0),
		m_editor(0),
		m_engine(0)
#endif // #ifdef INGAME_EDITOR
#ifdef PROFILE_CRITICAL_SECTIONS
		,mt_csEnter(MUTEX_PROFILE_ID(CRenderDevice::mt_csEnter))
		,mt_csLeave(MUTEX_PROFILE_ID(CRenderDevice::mt_csLeave))
#endif // #ifdef PROFILE_CRITICAL_SECTIONS
	{
		m_hWnd = NULL;
		b_is_Active = FALSE;
		b_is_Ready = FALSE;
		Timer.Start();
		m_bNearer = FALSE;
	}

	void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
	BOOL Paused() const;

	// Scene control
	void PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);
	bool Begin();
	void Clear();
	void End();
	void FrameMove();

	void overdrawBegin();
	void overdrawEnd();

	// Mode control
	void DumpFlags();
	IC CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
	u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
	u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }

	// Creation & Destroying
	void ConnectToRender();
	void Create(void);
	void Run(void);
	void Destroy(void);
	void Reset(bool precache = true);

	void Initialize(void);
	void ShutDown(void);

public:
	void time_factor(const float& time_factor)
	{
		Timer.time_factor(time_factor);
		TimerGlobal.time_factor(time_factor);
	}

	IC const float& time_factor() const
	{
		VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
		return (Timer.time_factor());
	}

	// Multi-threading
	xrCriticalSection mt_csEnter;
	xrCriticalSection mt_csLeave;
	volatile BOOL mt_bMustExit;

	ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<>& delegate)
	{
		xr_vector<fastdelegate::FastDelegate0<> >::iterator I = std::find(
					seqParallel.begin(),
					seqParallel.end(),
					delegate
				);
		if (I != seqParallel.end())
			seqParallel.erase(I);
	}

public:
	void xr_stdcall on_idle();
	bool xr_stdcall on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

private:
	void message_loop();
	virtual void _BCL AddSeqFrame(pureFrame* f, bool mt);
	virtual void _BCL RemoveSeqFrame(pureFrame* f);
	virtual CStatsPhysics* _BCL StatPhysics() { return Statistic; }
#ifdef INGAME_EDITOR
public:
	IC editor::ide* editor() const { return m_editor; }

private:
	void initialize_editor();
	void message_loop_editor();

private:
	typedef editor::initialize_function_ptr initialize_function_ptr;
	typedef editor::finalize_function_ptr finalize_function_ptr;

private:
	HMODULE m_editor_module;
	initialize_function_ptr m_editor_initialize;
	finalize_function_ptr m_editor_finalize;
	editor::ide* m_editor;
	engine_impl* m_engine;
#endif // #ifdef INGAME_EDITOR

private:
	class ENGINE_API CSVP final
	{
	private:
		bool							m_active								= false;
		bool							m_rendering								= false;
		Fvector							m_position								= vZero;
		float							m_zoom									= 1.f;
		float							m_zoom_opposite_sqr						= 1.f;
		float							m_fov									= 0.f;
		float							m_view_fov								= 0.f;
		float							m_lense_fov								= 0.f;
		Fvector							m_lense_dir								= vZero;
		CFrustum						m_lense_view							= {};

	public:

		void							setRendering							(bool val)				{ m_rendering = val; }
		void							setPosition								(Fvector CR$ val)		{ m_position = val; }
		void							setFOV									(float val)				{ m_fov = val; }
		void							setViewFOV								(float val)				{ m_view_fov  = val; }
		void							setLenseFOV								(float val)				{ m_lense_fov = val; }
		
		void							setZoom									(float val);
		void							setActive								(bool val);
		void							setLenseDir								(Fvector CR$ val);

		bool							isActive							C$	()		{ return m_active; }
		bool							isRendering							C$	()		{ return m_rendering; }
		Fvector CR$						getPosition							C$	()		{ return m_position; }
		float							getZoom								C$	()		{ return m_zoom; }
		float							getZoomOppositeSqr					C$	()		{ return m_zoom_opposite_sqr; }
		float							getFOV								C$	()		{ return m_fov; }
		float							getViewFOV							C$	()		{ return m_view_fov; }
		float							getLenseFOV							C$	()		{ return m_lense_fov; }
		Fvector CR$						getLenseDir							C$	()		{ return m_lense_dir; }

		CFrustum&						lenseView								()		{ return m_lense_view; }
	};

private:
	void								render_svp								();
	void								render									();

public:
	CSVP								SVP;
	float								gFOV									= 75.f;
	float								aimFOV									= 36.f;
	float								aimFOVTan								= .32491969623290632615587141221513f;
	
	bool								isLevelReady						C$	();
	bool								isGameLoaded						C$	();
	bool _BCL							isActiveMain						CO$	();
	bool _BCL							isGameProcess						CO$	();
};

extern ENGINE_API CRenderDevice Device;

#ifndef _EDITOR
#define RDEVICE Device
#else
#define RDEVICE EDevice
#endif

extern ENGINE_API bool g_bBenchmark;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

class ENGINE_API CLoadScreenRenderer :public pureRender
{
private:
	bool b_registered = false;
	bool b_need_user_input = false;
	bool b_standby = false;

public:
	void start(bool b_user_input);
	void stop();
	void OnRender() override;
	bool getStandby() const { return b_standby; }
	void setStandby() { b_standby = true; }
	bool isActive() const { return b_registered; }
	bool needUserInput() const { return b_need_user_input; }
};

extern ENGINE_API CLoadScreenRenderer load_screen_renderer;
