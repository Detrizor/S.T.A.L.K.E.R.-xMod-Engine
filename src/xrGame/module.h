#pragma once

class CGameObject;
class CInventoryItemObject;

enum EEventTypes
{
	//CObject
	eRenderableRender,
	//CGameObject
	eOnChild,
	eSyncData,
	//MAddonOwner
	eOnAddon,
	eTransferAddon,
	//CInventoryItem
	eGetAmount,
	eGetFill,
	eGetBar,
	eWeight,
	eVolume,
	eCost,
	eInstallUpgrade,
	//CHudItem
	eUpdateHudBonesVisibility,
	eRenderHudMode,
	eUpdateSlotsTransform
};

class CModule
{
public:
	CGameObject&						O;
	CInventoryItemObject PC$			I;

public:
										CModule									(CGameObject* obj);

public:
	float							V$	aboba									(EEventTypes type, void* data = NULL, int additional_data = 0)		{ return flt_max; }

public:
	void								Transfer							C$	(u16 id = u16_max);
};
