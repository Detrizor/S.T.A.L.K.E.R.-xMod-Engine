#pragma once

struct	firedeps
{
	Fmatrix				m_FireParticlesXForm;	//направление для партиклов огня и дыма
	Fvector				vLastFP; 				//fire point global
	Fvector				vLastFP2;				//fire point2

	firedeps			()
	{
		m_FireParticlesXForm.identity();
		vLastFP = vLastFP2 = vZero;
	}
};
