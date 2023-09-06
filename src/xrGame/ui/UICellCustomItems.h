#pragma once
#include "UICellItem.h"
#include "../Weapon.h"

class CAddonOwner;
struct SAddonSlot;

struct SIconLayer
{
	LPCSTR m_name;
	CUIStatic* m_icon;
	Fvector2 offset;
	//u32 m_color;
	float m_scale;
};

class CUIInventoryCellItem :public CUICellItem
{
	typedef  CUICellItem	inherited;
public:
											CUIInventoryCellItem		(CInventoryItem* itm);
											CUIInventoryCellItem		(shared_str section);
	virtual		bool						EqualTo						(CUICellItem* itm);
	virtual		void						UpdateItemText				();
				CUIDragItem*				CreateDragItem				();
	virtual		bool						IsHelper					();
	virtual		void						SetIsHelper					(bool is_helper);
				bool						IsHelperOrHasHelperChild	();
				void						Update						();
				CInventoryItem*				object						() {return (CInventoryItem*)m_pData;}
				//Alundaio
				virtual		void			OnAfterChild(CUIDragDropListEx* parent_list);
				virtual		void			SetTextureColor(u32 color);

				xr_vector<SIconLayer*>		m_layers;
				void						RemoveLayer					(SIconLayer* layer);
				void						CreateLayer					(LPCSTR name, Fvector2 offset, float scale);
				CUIStatic*					InitLayer					(CUIStatic* s, LPCSTR section, Fvector2 addon_offset, bool b_rotate, float scale);
				//-Alundaio
};

class CUIAmmoCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			 UpdateItemText				();
public:
								 CUIAmmoCellItem			(CWeaponAmmo* itm);

				u32				 CalculateAmmoCount			();
	virtual		bool			 EqualTo						(CUICellItem* itm);
	virtual		CUIDragItem*	 CreateDragItem				();
				CWeaponAmmo*	 object						() { return smart_cast<CWeaponAmmo*>((CInventoryItem*)m_pData); }
};

class CUIAddonOwnerCellItem :public CUIInventoryCellItem
{
private:
	typedef CUIInventoryCellItem		inherited;

	struct SUIAddonSlot
	{
		shared_str						name;
		shared_str						type;
		shared_str						addon_name;
		u8								addon_type;
		u8								addon_index;
		CUIStatic*						addon_icon;
		Fvector2						icon_offset;

		SUIAddonSlot					(SAddonSlot CR$ slot);
	};

	typedef xr_vector<SUIAddonSlot*>	VUISlots;

public:
										CUIAddonOwnerCellItem					(CAddonOwner* item);
										~CUIAddonOwnerCellItem					();

private:
	VUISlots							m_slots;

	void								InitAddon								(CUIStatic* s, LPCSTR section, u8 type, u8 index, Fvector2 offset, bool use_heading, bool drag = false);

public:
	VUISlots CR$						Slots								C$	()		{ return m_slots; }

	bool								EqualTo								O$	(CUICellItem* itm) { return false; }

	void								Update								O$	();
	void								Draw								O$	();
	CUIDragItem*						CreateDragItem						O$	();
	void								SetTextureColor						O$	(u32 color);
	void								OnAfterChild						O$	(CUIDragDropListEx* parent_list);
};

class CBuyItemCustomDrawCell :public ICustomDrawCellItem
{
	CGameFont*			m_pFont;
	string16			m_string;
public:
						CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont);
	virtual void		OnDraw					(CUICellItem* cell);

};
