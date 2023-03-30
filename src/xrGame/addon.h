#pragma once
#include "module.h"

class CAddon : public CModule
{
public:
										CAddon									(CGameObject* obj);

private:
	shared_str							m_SlotType;

public:
	shared_str CR$						SlotType							C$	()		{ return m_SlotType; }

	void								Render									(Fmatrix* pos);
};
