#pragma once

constexpr unsigned short NO_ID = std::numeric_limits<unsigned short>::max();
constexpr float no_float = std::numeric_limits<float>::max();

class CGameObject;
class CInventoryItem;
class CObject;
class CAddon;

struct SAddonSlot;

enum EEventTypes
{
	//CGameObject
	OnChild,
	//CAddonOwner
	ProcessAddon,
	TransferAddon,
	//CInventoryItem
	GetAmount,
	GetFill,
	GetBar,
	Weight,
	Volume,
	Cost,
	//CHudItem
	UpdateHudBonesVisibility
};

class CInterface
{
public:
	void							V$	_OnChild								(CObject* obj, bool take) {}
	void							V$	_ProcessAddon							(CAddon CPC addon, bool attach, SAddonSlot CPC slot) {}
	int								V$	_TransferAddon							(CAddon CPC addon, bool attach) { return 0; }

	float							V$	Event									(EEventTypes type, void* data, u32 additional_data = 0) { return no_float; }
	float							V$	EventC								C$	(EEventTypes type, void* data, u32 additional_data = 0) { return no_float; }
};

class CModule : public CInterface
{
public:
	CGameObject&						O;

public:
										CModule									(CGameObject* obj) : O(*obj) {}

public:
	float							V$	_GetAmount							C$	()		{ return no_float; }
	float							V$	_GetFill							C$	()		{ return no_float; }
	float							V$	_GetBar								C$	()		{ return no_float; }

	float							V$	_Weight								C$	()		{ return 0.f; }
	float							V$	_Volume								C$	()		{ return 0.f; }
	float							V$	_Cost								C$	()		{ return 0.f; }

	void							V$	_UpdateHudBonesVisibility				()		{};

	float							V$	MEvent									(EEventTypes type, void* data, u32 additional_data = 0) { return no_float; }
	float							V$	MEventC								C$	(EEventTypes type, void* data, u32 additional_data = 0) { return no_float; }

public:
	void								Transfer							C$	(u16 id = NO_ID);
	template <typename T>
	T									cast								C$	() { return O.cast<T>(); }
	template <typename T>
	T									cast									() { return O.cast<T>(); }
};
