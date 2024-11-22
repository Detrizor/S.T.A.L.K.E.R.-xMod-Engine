#pragma once

class CPowerDependency
{
private:
	Fvector3							p1										= vZero;
	float								t										= 0.f;
	Fvector3							p2										= vZero;

public:
	void								Load(LPCSTR sect, LPCSTR line)
	{
		LPCSTR C						= pSettings->r_string(sect, line);
		if (sscanf(C, "%f,%f,%f,%f,%f,%f,%f", &p1.x, &p1.y, &p1.z, &t, &p2.x, &p2.y, &p2.z) < 7)
			p2							= p1;
	}

	float								Calc								C$	(float param)
	{
		Fvector3 CR$ p					= (param < t) ? p1 : p2;
		return							pow(p[0] + p[1] * param, p[2]);
	}
};
