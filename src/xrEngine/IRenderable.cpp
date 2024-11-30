#include "stdafx.h"
#include "../xrcdb/ispatial.h"
#include "irenderable.h"

constexpr float distance_update_period = 1.f;

IRenderable::IRenderable()
{
	renderable.xform.identity();
	renderable.visual = NULL;
	renderable.pROS = NULL;
	renderable.pROS_Allowed = TRUE;
	ISpatial* self = dynamic_cast<ISpatial*> (this);
	if (self) self->spatial.type |= STYPE_RENDERABLE;
}

extern ENGINE_API BOOL g_bRendering;
IRenderable::~IRenderable()
{
	VERIFY(!g_bRendering);
	Render->model_Delete(renderable.visual);
	if (renderable.pROS) Render->ros_destroy(renderable.pROS);
	renderable.visual = NULL;
	renderable.pROS = NULL;
}

IRender_ObjectSpecific* IRenderable::renderable_ROS()
{
	if (0 == renderable.pROS && renderable.pROS_Allowed) renderable.pROS = Render->ros_create(this);
	return renderable.pROS;
}

float IRenderable::calc_distance_to_camera() const
{
	return								Device.camera.position.distance_to_sqr(renderable.xform.c);
}

float IRenderable::get_distance_to_camera_base()
{
	if (m_next_distance_update_time < Device.fTimeGlobal)
	{
		m_distance						= calc_distance_to_camera();
		m_next_distance_update_time		= Device.fTimeGlobal + distance_update_period;
		on_distance_update				();
	}
	return								m_distance;
}

float IRenderable::getDistanceToCamera()
{
	return								get_distance_to_camera_base() * Device.camera.izoom_sqr;
}
