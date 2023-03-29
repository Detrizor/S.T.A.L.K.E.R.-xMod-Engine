#pragma once
#include "module.h"

class CItemStorage : public CModule
{
public:
										CItemStorage							(CGameObject* obj) : CModule(obj) {}

public:
	void								OnEvent								O$	(NET_Packet& P, u16 type);
};
