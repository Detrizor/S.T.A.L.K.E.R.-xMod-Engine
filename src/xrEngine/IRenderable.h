#pragma once

#include "render.h"

//////////////////////////////////////////////////////////////////////////
// definition ("Renderable")
class ENGINE_API IRenderable
{
public:
	struct
	{
		Fmatrix xform;
		IRenderVisual* visual;
		IRender_ObjectSpecific* pROS;
		BOOL pROS_Allowed;
	} renderable;
public:
	IRenderable();
	virtual ~IRenderable();
	IRender_ObjectSpecific* renderable_ROS();
	BENCH_SEC_SCRAMBLEVTBL2
		virtual void renderable_Render() = 0;
	virtual BOOL renderable_ShadowGenerate() { return FALSE; };
	virtual BOOL renderable_ShadowReceive() { return FALSE; };

private:
	float								m_distance								= 0.f;
	float								m_next_distance_update_time				= 0.f;

	virtual void						on_distance_update						() {}
	virtual float						calc_distance_to_camera				C$	();

protected:
	float								get_distance_to_camera_base				();

public:
	float								getDistanceToCamera						();
};
