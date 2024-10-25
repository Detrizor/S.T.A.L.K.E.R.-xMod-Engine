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
	u32									m_last_distance_calc_frame						= 0;
	float								m_last_distance									= 0.f;

	virtual float						get_distance_to_camera						C$	();

public:
	float								getDistanceToCamera								();
};
