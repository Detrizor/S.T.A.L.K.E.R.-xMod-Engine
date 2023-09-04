#pragma once
#include "module.h"

class CAddon;
class CAddonOwner;
struct SAddonSlot;

typedef xr_vector<SAddonSlot*> VSlots;

struct SAddonSlot
{
	CAddonOwner PC$						parent_ao;
	SAddonSlot PC$						forwarded_slot;

	u16									idx;
	shared_str							name;
	shared_str							type;
	shared_str							bone_name;
	Fvector								model_offset[2];
	Fvector								bone_offset[2];
	Fvector2							icon_offset;
	BOOL								lower_iron_sights;
	BOOL								alt_scope;
	BOOL								magazine;
	u16									overlaping_slot;

	CAddon*								addon;
	CAddon*								loading_addon;
	Fmatrix								render_pos;

										SAddonSlot								(LPCSTR section, u16 _idx, CAddonOwner PC$ parent);
										SAddonSlot								(SAddonSlot PC$ slot, SAddonSlot CPC root_slot, CAddonOwner PC$ parent);

	void								UpdateRenderPos							(IRenderVisual* model, Fmatrix parent);
	void								RenderHud								();
	void								RenderWorld								(IRenderVisual* model, Fmatrix parent);

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
