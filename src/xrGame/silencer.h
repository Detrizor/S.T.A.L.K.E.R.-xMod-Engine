#pragma once

#include "module.h"

class CGameObject;

class CSilencer : public CModule
{
public:
										CSilencer								(CGameObject* obj) : CModule(obj) {}
	void								Load									(LPCSTR section);
};
