#pragma once

struct	firedeps
{
	Fmatrix				m_FireParticlesXForm;	//направление для партиклов огня и дыма
	Fvector				vLastFPLocal; 			//fire point in local coordinates
	Fvector				vLastFP; 				//fire point global
	Fvector				vLastFP2;				//fire point2
	Fvector				vLastFD;				//fire direction
	Fvector				vLastSP;				//shell point	

	firedeps			()
	{
		m_FireParticlesXForm.identity();
		vLastFPLocal = vLastFP = vLastFP2 = vLastFD = vLastSP = vZero;
	}
};
