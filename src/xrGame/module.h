#pragma once
#include "signal_processor.h"

class CGameObject;
class CInventoryItemObject;

class CModule : public CSignalProcessor
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
		mArtefactModule,
		mModuleTypesEnd
	};

protected:
	explicit CModule(CGameObject* obj);

public:
	virtual ~CModule() = default;
	CModule(CModule&&) = delete;

public:
	CGameObject& O;
	CInventoryItemObject* const I;
};
