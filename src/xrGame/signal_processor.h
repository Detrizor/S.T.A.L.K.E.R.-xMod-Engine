#pragma once

enum EEventTypes
{
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

class CGameObject;
class CSE_ALifeDynamicObject;

class CSignalProcessor
{
public:
	//CGameObject
	virtual void						sOnChild								(CGameObject* obj, bool take)					{}
	virtual void						sSyncData								(CSE_ALifeDynamicObject* se_obj, bool save)		{}
	virtual void						sRenderableRender						()												{}
};
