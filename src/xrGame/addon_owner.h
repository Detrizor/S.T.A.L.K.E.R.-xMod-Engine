#pragma once
#include "module.h"

class CAddon;
struct SAddonSlot;

typedef xr_vector<SAddonSlot*> VSlots;

struct SAddonSlot
{
	VSlots CR$							parent_slots;

	u16									idx;
	shared_str							name;
	shared_str							type;
	shared_str							bone_name;
	Fvector								model_offset[2];
	Fvector								bone_offset[2];
	Fvector2							icon_offset;
	BOOL								lower_iron_sights;
	BOOL								primary_scope;
	BOOL								magazine;
	u16									overlaping_slot;

	CAddon*								addon;
	CAddon*								loading_addon;
	Fmatrix								render_pos;		//Текущая позиция для рендеринга

										SAddonSlot								(LPCSTR section, u16 _idx, VSlots CR$ slots);

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
	u16									m_NextAddonSlot;
	
	void								LoadAddonSlots							(LPCSTR section);
	int									TransferAddon							(CAddon CPC addon, bool attach);

	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	void								ModifyControlInertionFactor			C$	(float& cif);

	int									AttachAddon								(CAddon CPC addon, u16 slot_idx = u16_max);
	int									DetachAddon								(CAddon CPC addon);
	void								UpdateSlotsTransform					();
			
	void								renderable_Render						();
	void								render_hud_mode							();
};
