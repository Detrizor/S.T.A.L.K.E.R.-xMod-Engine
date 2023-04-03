#pragma once

#include "module.h"

class CGameObject;

class CSilencer : public CModule
{
public:
										CSilencer								(CGameObject* obj, shared_str CR$ section);

private:
	shared_str							m_section;

public:
	shared_str CR$						Section								C$	()		{ return m_section; }
};
