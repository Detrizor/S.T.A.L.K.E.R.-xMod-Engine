#pragma once
#include "module.h"

class CAddon;
class CAddonOwner;
class CAddonSlot;

typedef xr_vector<CAddonSlot*> VSlots;

class CAddonSlot
{
public:
										CAddonSlot								(LPCSTR section, u16 _idx, CAddonOwner PC$ parent);

private:
	CAddon CPC							parent_addon;

	float								step;
	u16									bone_id;
	Fmatrix								bone_offset;
	u16									overlaping_slot;
	u16									loading_bone_id;
	Fmatrix								loading_model_offset;
	Fmatrix								loading_bone_offset;

	void								append_bone_trans					C$	(Fmatrix& trans, IKinematics* model, u16 bone, Fmatrix CR$ parent_trans);
	int									get_spacing							C$	(CAddon CPC left, CAddon CPC right);
	CAddon*								get_next_addon						C$	(xr_list<CAddon*>::iterator& I);
	CAddon*								get_prev_addon						C$	(xr_list<CAddon*>::iterator& I);

public:
	CAddonOwner PC$						parent_ao;
	const u16							idx;

	shared_str							name;
	shared_str							type;
	int									steps;
	Fmatrix								model_offset;
	Fvector2							icon_offset;
	float								icon_step;
	u8									blocking_iron_sights;		//1 for blocking if non-lowered addon attached, 2 for force block on any addon
	BOOL								magazine;
	bool								muzzle;

	xr_list<CAddon*>					addons									= {};
	CAddon*								loading_addon							= NULL;

	void								attachAddon								(CAddon* addon);
	void								attachLoadingAddon						(CAddon* addon);
	void								detachAddon								(CAddon* addon);
	void								detachLoadingAddon						();

	void								updateAddonsHudTransform				(IKinematics* model, Fmatrix CR$ parent_trans);
	void								shiftAddon								(CAddon* addon, int shift);

	void								RenderHud							C$	();
	void								RenderWorld							C$	(IRenderVisual* model, Fmatrix CR$ parent_trans);
	bool								Compatible							C$	(CAddon CPC addon);
	bool								CanTake								C$	(CAddon CPC addon);
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
	
	CAddonSlot*							find_available_slot					C$	(CAddon* addon);

	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	CAddonOwner*						getParentAO							C$	();

	int									AttachAddon								(CAddon* addon, CAddonSlot* slot = NULL);
	int									DetachAddon								(CAddon* addon);
	void								RegisterAddon							(CAddon PC$ addon, bool attach);

	void							S$	LoadAddonSlots							(LPCSTR section, VSlots& slots, CAddonOwner PC$ parent_ao = NULL);
};
