#pragma once

class CInventoryItem;

class CModule
{
protected:
	CGameObject PC$						pO;
	CGameObject&						O;

public:
										CModule									(CGameObject* obj) : pO(obj), O(*obj) {}

public:
	void							V$	OnEvent									(NET_Packet& P, u16 type) {}
	void							V$	OnChild									(CObject* obj, bool take) {}
};
