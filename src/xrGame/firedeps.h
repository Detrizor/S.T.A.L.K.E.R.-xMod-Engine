#pragma once

struct	firedeps
{
	Fmatrix				m_FireParticlesXForm;	//����������� ��� ��������� ���� � ����
	Fvector				vLastFP; 				//fire point global
	Fvector				vLastFP2;				//fire point2

	firedeps			()
	{
		m_FireParticlesXForm.identity();
		vLastFP = vLastFP2 = vZero;
	}
};
