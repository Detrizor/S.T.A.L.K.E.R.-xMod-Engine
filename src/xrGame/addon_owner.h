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
	Fvector2							icon_offset;
	BOOL								icon_on_background;
	BOOL								lower_iron_sights;
	BOOL								primary_scope;
	u16									overlaping_slot;

	CAddon*								addon;
	Fmatrix								render_pos;		//Текущая позиция для рендеринга

										SAddonSlot								(LPCSTR section, u16 _idx, VSlots CR$ slots);

	void								UpdateRenderPos							(IRenderVisual* model, Fmatrix parent);
	void								RenderHud								();
	void								RenderWorld								(IRenderVisual* model, Fmatrix parent);

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

protected:
	void								OnChild								O$	(CObject* obj, bool take);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	void								ModifyControlInertionFactor			C$	(float& cif);

	bool								AttachAddon								(CAddon CPC addon, u16 slot_idx = NO_ID);
	void								DetachAddon								(CAddon CPC addon);
	void								UpdateSlotsTransform					();
			
	void								renderable_Render						();
	void								render_hud_mode							();

	void								ProcessAddon						O$	(CAddon CPC addon, bool attach, SAddonSlot CPC slot);
	bool								TransferAddon						O$	(CAddon CPC addon, bool attach);

	float								Weight								CO$	();
	float								Volume								CO$	();
	float								Cost								CO$	();
};
