#pragma once


class CModule
{
protected:
	CGameObject PC$						pO;
	CGameObject&						O;

public:
										CModule									(CGameObject* obj) : pO(obj), O(*obj) {}

public:
	void							V$	OnEvent									(NET_Packet& P, u16 type) {}
	void							V$	OnEventImpl								(u16 type, u16 id, CObject* itm, bool dont_create_shell) {}
};
