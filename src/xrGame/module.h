#pragma once

class CGameObject;

enum EEventTypes
{
	//CGameObject
	eOnChild,
	//CAddonOwner
	eOnAddon,
	eTransferAddon,
	//CInventoryItem
	eGetAmount,
	eGetFill,
	eGetBar,
	eWeight,
	eVolume,
	eCost,
	//CHudItem
	eUpdateHudBonesVisibility
};

class CModule
{
public:
	CGameObject&						O;

public:
										CModule									(CGameObject* obj) : O(*obj) {}

public:
	float							V$	aboba									(EEventTypes type, void* data = NULL, int additional_data = 0) { return flt_max; }

public:
	void								Transfer							C$	(u16 id = u16_max);
	template <typename T, typename C>
	T								S$	cast									(C c)					{ return CObject::Cast<T>(c); }
	template <typename T>
	T									cast								C$	()						{ return O.Cast<T>(); }
	template <typename T>
	T									cast									()						{ return O.Cast<T>(); }
};
