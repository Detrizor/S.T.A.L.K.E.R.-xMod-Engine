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
