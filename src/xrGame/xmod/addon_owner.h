#pragma once
#include "../InventoryBox.h"

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
			bool			CanTake					(const CAddonObject* const _addon);
	const	Fvector*		GetModelOffset			();
};

class CAddonOwner : public CInventoryStorage
{
private:
	typedef	CInventoryStorage inherited;
			CGameObject*	m_object;

public:
							CAddonOwner				();
							~CAddonOwner			() {}
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);

private:
	xr_vector<SAddonSlot>	m_Slots;
			u16				m_iNextAddonSlot;

	IC		void			AddonAttach				(CAddon* const addon, SAddonSlot* slot = NULL) { ProcessAddon(addon, TRUE, slot); }
	IC		void			AddonDetach				(CAddon* const addon, SAddonSlot* slot = NULL) { ProcessAddon(addon, FALSE, slot); }
	
			void			LoadAddonSlots			(LPCSTR section);

protected:
	virtual	void			OnEventImpl				(u16 type, u16 id, CObject* itm, bool dont_create_shell);
	virtual	void			ProcessAddon			(CAddon* const addon, BOOL attach, const SAddonSlot* const slot) {};
	
public:
	IC const xr_vector<SAddonSlot>& AddonSlots		()								const	{ return m_Slots; }

			void			AttachAddon				(CAddonObject* addon, u16 slot_idx = NO_ID);
			void			DetachAddon				(CAddonObject* addon);

	virtual	void			TransferAnimation		(CAddonObject* addon, bool attach);
			
    virtual void			renderable_Render		();
	virtual void			UpdateAddonsTransform	();
    virtual void			render_hud_mode			();
	virtual	float			GetControlInertionFactor()								const;
};
