#pragma once
#include "module.h"

class CAddon;
class CAddonOwner;
struct SAddonSlot;

typedef xr_vector<SAddonSlot*> VSlots;

struct SAddonSlot
{
	CAddonOwner PC$						parent_ao;
	CAddon CPC							parent_addon;
	
	Fmatrix								loading_transform						= Fidentity;
	xr_list<CAddon*>					addons									= {};
	CAddon*								loading_addon							= NULL;

	u16									idx;
	shared_str							name;
	shared_str							type;
	float								length;
	float								steps;
	u16									bone_id;
	Fmatrix								model_offset;
	Fmatrix								bone_offset;
	Fvector2							icon_offset;
	u8									blocking_iron_sights;		//1 for blocking if non-lowered addon attached, 2 for force block on any addon
	u16									overlaping_slot;
	bool								muzzle;

	BOOL								magazine;
	u16									loading_bone_id;
	Fmatrix								loading_model_offset;
	Fmatrix								loading_bone_offset;

										SAddonSlot								(LPCSTR section, u16 _idx, CAddonOwner PC$ parent);
										
	void								append_bone_trans					C$	(Fmatrix& trans, IKinematics* model, u16 bone, Fmatrix CR$ parent_trans);
	
	void								attachAddon								(CAddon* addon);
	void								attachLoadingAddon						(CAddon* addon);
	void								detachAddon								(CAddon* addon);
	void								detachLoadingAddon						();

	void								updateAddonsHudTransform				(IKinematics* model, Fmatrix CR$ parent_trans);
	
	bool								hasAddon							C$	(CAddon CPC _addon)		{ return (::std::find(addons.begin(), addons.end(), _addon) != addons.end()); }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(IRenderVisual* model, Fmatrix CR$ parent_trans);
	bool								Compatible							C$	(CAddon CPC addon);
	bool								CanTake								C$	(CAddon CPC addon);
	void								shiftAddon							C$	(CAddon* addon, int shift);
	void								updateAddonLocalTransform			C$	(CAddon* addon);
};

class CAddonOwner : public CModule
{
public:
										CAddonOwner								(CGameObject* obj);
										~CAddonOwner							();

private:
	VSlots								m_Slots;
	
	void								LoadAddonSlots							(LPCSTR section);
	int									TransferAddon							(CAddon CPC addon, bool attach);

	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	CAddonOwner*						getParentAO							C$	();

	int									AttachAddon								(CAddon* addon, SAddonSlot* slot = NULL);
	int									DetachAddon								(CAddon* addon);
	void								RegisterAddon							(CAddon PC$ addon, bool attach);

	void							S$	LoadAddonSlots							(LPCSTR section, VSlots& slots, CAddonOwner PC$ parent_ao = NULL);
};
