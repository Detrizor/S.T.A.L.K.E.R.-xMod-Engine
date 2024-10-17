#pragma once

enum EEventTypes
{
	//CGameObject
	eOnChild,
	eSyncData,
	eRenderableRender,

	//MAddonOwner
	eOnAddon,

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

class CSignalProcessor
{
};
