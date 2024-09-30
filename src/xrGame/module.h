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
	enum EModuleTypes
	{
		mModuleTypesBegin,
		mAddon = mModuleTypesBegin,
		mAddonOwner,
		mFoldable,
		mLauncher,
		mInventoryItem,
		mAmountable,
		mContainer,
		mUsable,
		mMagazine,
		mMuzzle,
		mScope,
		mModuleTypesEnd
	};

	CGameObject&						O;
	CInventoryItemObject PC$			I;

protected:
										CModule									(CGameObject* obj);

public:
	float							V$	aboba									(EEventTypes type, void* data = nullptr, int additional_data = 0)		{ return flt_max; }
};
