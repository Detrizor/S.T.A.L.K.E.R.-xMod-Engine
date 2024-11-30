////////////////////////////////////////////////////////////////////////////
//	Module 		: script_render_device_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script render device script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_render_device.h"

using namespace ::luabind;

bool is_device_paused(CRenderDevice* d)
{
	return								!!Device.Paused();
}

void set_device_paused(CRenderDevice* d, bool b)
{
	Device.Pause						(b, TRUE, FALSE,"set_device_paused_script");
}

extern ENGINE_API BOOL g_appLoaded;
bool is_app_ready()
{
	return								!!g_appLoaded;
}

u32 time_global(const CRenderDevice *self)
{
	THROW								(self);
	return								(self->dwTimeGlobal);
}

Fvector CR$ cam_pos(CRenderDevice CP$ self)
{
	return								self->camera.position;
}

Fvector CR$ cam_dir(CRenderDevice CP$ self)
{
	return								self->camera.direction;
}

Fvector CR$ cam_top(CRenderDevice CP$ self)
{
	return								self->camera.top;
}

Fvector CR$ cam_right(CRenderDevice CP$ self)
{
	return								self->camera.right;
}

float cam_fov(CRenderDevice CP$ self)
{
	return								self->camera.fov;
}

float cam_aspect(CRenderDevice CP$ self)
{
	return								self->camera.aspect;
}

#pragma optimize("s",on)
void CScriptRenderDevice::script_register(lua_State *L)
{
	module(L)
	[
		class_<CRenderDevice>("render_device")
			.def_readonly("width",					&CRenderDevice::dwWidth)
			.def_readonly("height",					&CRenderDevice::dwHeight)
			.def_readonly("time_delta",				&CRenderDevice::dwTimeDelta)
			.def_readonly("f_time_delta",			&CRenderDevice::fTimeDelta)
			.def("time_global",						&time_global)
			.def_readonly("precache_frame",			&CRenderDevice::dwPrecacheFrame)
			.def_readonly("frame",					&CRenderDevice::dwFrame)
			.def("is_paused",						&is_device_paused)
			.def("pause",							&set_device_paused)
			
			.def("cam_pos",							&cam_pos)
			.def("cam_dir",							&cam_dir)
			.def("cam_top",							&cam_top)
			.def("cam_right",						&cam_right)
			.def("cam_fov",							&cam_fov)
			.def("cam_aspect",						&cam_aspect),

			def("app_ready",						&is_app_ready)
	];
}
