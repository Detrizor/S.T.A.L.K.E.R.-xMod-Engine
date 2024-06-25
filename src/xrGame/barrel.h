#pragma once
#include "muzzle.h"

class CBarrel : public CMuzzleBase
{
public:
										CBarrel									(CGameObject* obj, shared_str CR$ section);

private:
	const float							m_length;

public:
	float								getLength							C$	()		{ return m_length; }
};
