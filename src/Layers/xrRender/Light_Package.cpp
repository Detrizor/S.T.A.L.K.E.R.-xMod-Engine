#include "stdafx.h"
#include "Light_Package.h"

void	light_Package::clear	()
{
	v_point.clear		();
	v_spot.clear		();
	v_shadowed.clear	();
}

IC	bool	pred_light_cmp	(light*	_1, light* _2)
{
	if	(_1->vis.pending)
	{
		if (_2->vis.pending)	return	_1->vis.query_order > _2->vis.query_order;	// q-order
		else					return	false;										// _2 should be first
	} else {
		if (_2->vis.pending)	return	true;										// _1 should be first 
		else					return	_1->range > _2->range;						// sort by range
	}
}

void	light_Package::sort		()
{
	// resort lights (pending -> at the end), maintain stable order
	v_point.sort(pred_light_cmp);
	v_spot.sort(pred_light_cmp);
	v_shadowed.sort(pred_light_cmp);
}
