#pragma once
#include "signal_processor.h"

class CGameObject;
class CInventoryItemObject;

class CModule : virtual public CSignalProcessor
{
public:
	enum EModuleTypes
	{
		mModuleTypesBegin,
		mAmountable = mModuleTypesBegin,
		mContainer,
		mUsable,
		mAddonOwner,
		mAddon,
		mScope,
		mMuzzle,
		mSilencer,
		mLauncher,
		mMagazine,
		mFoldable,
		mInventoryItem,
		mModuleTypesEnd
	};

	CGameObject&						O;
	CInventoryItemObject PC$			I;

protected:
										CModule									(CGameObject* obj);
};
