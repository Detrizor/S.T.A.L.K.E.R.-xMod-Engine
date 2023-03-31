#pragma once

constexpr unsigned short NO_ID = std::numeric_limits<unsigned short>::max();
constexpr float no_float = std::numeric_limits<float>::max();

class CGameObject;
class CInventoryItem;
class CObject;
class CAddon;

struct SAddonSlot;

class CInterface
{
public:
	void							V$	OnChild									(CObject* obj, bool take) {}
	void							V$	ProcessAddon							(CAddon CPC addon, bool attach, SAddonSlot CPC slot) {}
	bool							V$	TransferAddon							(CAddon CPC addon, bool attach) { return false; }

	float							V$	GetAmount							C$	()		{ return no_float; }
	float							V$	GetFill								C$	()		{ return no_float; }
	float							V$	GetBar								C$	()		{ return no_float; }

	float							V$	Weight								C$	()		{ return 0.f; }
	float							V$	Volume								C$	()		{ return 0.f; }
	float							V$	Cost								C$	()		{ return 0.f; }
};

class CModule : public CInterface
{
public:
	CGameObject PC$						pO;
	CGameObject&						O;
	CInventoryItem*						pI;

public:
										CModule									(CGameObject* obj);

public:
	void								Transfer							C$	(u16 id = NO_ID);
	
	template <typename T>
	T									cast								C$	() { return O.cast<T>(); }
	template <typename T>
	T									cast									() { return O.cast<T>(); }
};
