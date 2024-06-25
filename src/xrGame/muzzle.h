#pragma once
#include "module.h"

class CMuzzleBase : public CModule
{
public:
										CMuzzleBase								(CGameObject* obj, shared_str CR$ section);

private:
	const shared_str					m_section;
	const Fvector						m_muzzle_point;

public:
	shared_str CR$						getSection							C$	()		{ return m_section; }
	Fvector 							getFirePoint						C$	();
};

class CMuzzle : public CMuzzleBase
{
public:
										CMuzzle									(CGameObject* obj, shared_str CR$ section) : CMuzzleBase(obj, section) {}
};
