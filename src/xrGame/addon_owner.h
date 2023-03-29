#pragma once
#include "item_storage.h"
#include "inventory_space.h"

class CAddonObject;
class CAddon;

struct SAddonSlot
{
			u16				idx;
			shared_str		name;
			shared_str		type;
			Fvector			model_offset[2];
			Fvector2		icon_offset;
			bool			icon_on_background;
			bool			lower_iron_sights;
			bool			primary_scope;
			u16				overlaping_slot;
			CAddonObject*	addon;
			const xr_vector<SAddonSlot>& parent_slots;

							SAddonSlot				(LPCSTR _name, const xr_vector<SAddonSlot>& slots);
			void			Load					(LPCSTR section, u16 _idx);
			bool			CanTake					(CAddonObject CPC _addon) const;
};

typedef xr_vector<SAddonSlot> VSlots;

class CAddonOwner
{
private:
	CGameObject* m_object;

public:
												CAddonOwner								();
	DLL_Pure*									_construct								();

	virtual	void			Load					(LPCSTR section);

private:
	VSlots										m_Slots;
	u16											m_NextAddonSlot;

	void										AddonAttach								(CAddon* const addon, SAddonSlot* slot = NULL)	{ ProcessAddon(addon, TRUE, slot); }
	void										AddonDetach								(CAddon* const addon, SAddonSlot* slot = NULL)	{ ProcessAddon(addon, FALSE, slot); }
	
	void										LoadAddonSlots							(LPCSTR section);

protected:
	void									ModifyControlInertionFactor				C$	(float& cif);

	virtual	void			OnChild					(CObject* obj, bool take);
	virtual	void			ProcessAddon			(CAddon* const addon, BOOL attach, const SAddonSlot* const slot) {};
	
public:
	VSlots CR$								AddonSlots							C$	()		{ return m_Slots; }

	bool									AttachAddon								(CAddonObject CPC addon, u16 slot_idx = NO_ID);
	void									DetachAddon								(CAddonObject CPC addon);

	void								V$	TransferAnimation						(CAddonObject CPC addon, bool attach);
			
    virtual void			renderable_Render			();
	virtual void			UpdateAddonsTransform		();
    virtual void			render_hud_mode				();
};
