#pragma once

class CGameObject;
class CInventoryItemObject;

class CModule
{
protected:
	CGameObject PC$						pO;
	CGameObject&						O;

public:
										CModule									(CGameObject* obj) : pO(obj), O(*obj) {}

public:
	CGameObject CR$						Object								C$	()	{ return O; }
	CInventoryItemObject CR$			Item								C$	();

	void							V$	OnEvent									(NET_Packet& P, u16 type) {}
	void							V$	OnChild									(CObject* obj, bool take) {}
};
