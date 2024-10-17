#pragma once

enum EEventTypes
{
	//CInventoryItem
	eGetAmount,
	eGetFill,
	eGetBar,
	eWeight,
	eVolume,
	eCost,
	eInstallUpgrade,
};

class MAddon;
class CGameObject;
class CSE_ALifeDynamicObject;

class CSignalProcessor
{
public:
	//CGameObject
	virtual void						sOnChild								(CGameObject* obj, bool take)					{}
	virtual void						sSyncData								(CSE_ALifeDynamicObject* se_obj, bool save)		{}
	virtual void						sRenderableRender						()												{}

	//MAddonOwner
	virtual void						sOnAddon								(MAddon* addon, int attach_type)		{}

	//CHudItem
	virtual void						sUpdateHudBonesVisibility				()		{}
	virtual void						sRenderHudMode							()		{}
	virtual void						sUpdateSlotsTransform					()		{}
};
