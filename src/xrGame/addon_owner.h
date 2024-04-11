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
	SAddonSlot PC$						forwarded_slot;
	
	Fmatrix								transform								= Fidentity;
	Fmatrix								loading_transform						= Fidentity;
	CAddon*								addon									= NULL;
	CAddon*								loading_addon							= NULL;

	u16									idx;
	shared_str							name;
	shared_str							type;
	u16									bone_id;
	Fmatrix								model_offset;
	Fmatrix								bone_offset;
	Fvector2							icon_offset;
	BOOL								blocking_iron_sights;
	u16									overlaping_slot;
	bool								muzzle;

	BOOL								magazine;
	u16									loading_bone_id;
	Fmatrix								loading_model_offset;
	Fmatrix								loading_bone_offset;

										SAddonSlot								(LPCSTR section, u16 _idx, CAddonOwner PC$ parent);
										SAddonSlot								(SAddonSlot PC$ slot, SAddonSlot CPC root_slot, CAddonOwner PC$ parent);
										
	void								append_bone_trans					C$	(Fmatrix& trans, IKinematics* model, Fmatrix CPC parent_trans, u16 bone);
	
	void								registerAddon							(CAddon* addon_);
	void								registerLoadingAddon					(CAddon* addon_);
	void								updateAddonLocalTransform				();
	void								unregisterAddon							();
	void								unregisterLoadingAddon					();

	void								updateAddonHudTransform					(IKinematics* model, Fmatrix CR$ parent_trans);

	void								RenderHud							C$	();
	void								RenderWorld							C$	(IRenderVisual* model, Fmatrix CR$ parent_trans);
	bool								Compatible							C$	(CAddon CPC _addon);
	bool								CanTake								C$	(CAddon CPC _addon);
};

class CAddonOwner : public CModule
{
public:
										CAddonOwner								(CGameObject* obj);
										~CAddonOwner							();

private:
	VSlots								m_Slots;
	SAddonSlot*							m_NextAddonSlot;
	
	void								LoadAddonSlots							(LPCSTR section);
	int									TransferAddon							(CAddon CPC addon, bool attach);

	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	CAddonOwner*						ParentAO							C$	();

	int									AttachAddon								(CAddon CPC addon, SAddonSlot* slot = NULL);
	int									DetachAddon								(CAddon CPC addon);
	void								RegisterAddon							(CAddon PC$ addon, SAddonSlot PC$ slot, bool attach);

	void							S$	LoadAddonSlots							(LPCSTR section, VSlots& slots, CAddonOwner PC$ parent_ao = NULL);
};
