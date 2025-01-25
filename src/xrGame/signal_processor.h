#pragma once

class MAddon;
class CGameObject;
class CUICellItem;
class CSE_ALifeDynamicObject;

enum EItemDataTypes;

class CSignalProcessor
{
public:
	//CGameObject
	virtual void						sOnChild								(CGameObject* obj, bool take)					{}
	virtual void						sSyncData								(CSE_ALifeDynamicObject* se_obj, bool save)		{}
	virtual void						sRenderableRender						()												{}
	virtual void						sNetRelcase								(CObject* obj)									{}

	//MAddonOwner
	virtual void						sOnAddon								(MAddon* addon, int attach_type)		{}

	//CHudItem
	virtual void						sUpdateHudBonesVisibility				()		{}
	virtual void						sUpdateSlotsTransform					()		{}
	virtual void						sRenderHudMode							()		{}

	//CInventoryItem
	virtual bool						sInstallUpgrade							(LPCSTR section, bool test)		{ return false;}
	virtual float						sSumItemData							(EItemDataTypes type)			{ return 0.f; }
	virtual xoptional<float>			sGetAmount								()								{ return {}; }
	virtual xoptional<float>			sGetFill								()								{ return {}; }
	virtual xoptional<float>			sGetBar									()								{ return {}; }
	virtual xoptional<CUICellItem*>		sCreateIcon								()								{ return {}; }
};
