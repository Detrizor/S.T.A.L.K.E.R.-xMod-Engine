#pragma once

#include "module.h"

class CGrenadeLauncher : public CModule
{
public:
										CGrenadeLauncher						(CGameObject* obj);

	void								Load									(LPCSTR section);

private:
	float								m_fGrenadeVel;

public:
	float								GetGrenadeVel						C$	()		{ return m_fGrenadeVel; }
};
