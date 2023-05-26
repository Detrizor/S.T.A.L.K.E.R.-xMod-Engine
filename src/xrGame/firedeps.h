#pragma once

struct	firedeps
{
	Fmatrix				m_FireParticlesXForm;	//направление для партиклов огня и дыма
	Fvector				vLastFP; 				//fire point
	Fvector				vLastFP2;				//fire point2
	Fvector				vLastFD;				//fire direction
	Fvector				vLastFDD;				//fire direction default (con-compensated by zeroing)
	Fvector				vLastSP;				//shell point	

	firedeps			()
	{
		m_FireParticlesXForm.identity();
		vLastFP = vLastFP2 = vLastFD = vLastFDD = vLastSP = vZero;
	}
};
