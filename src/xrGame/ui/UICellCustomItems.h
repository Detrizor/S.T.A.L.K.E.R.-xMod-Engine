#pragma once
#include "UICellItem.h"
#include "../Weapon.h"

class CAddonSlot;
class MAddonOwner;
class MArtefactModule;

typedef xr_vector<xptr<CAddonSlot>> VSlots;

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
											CUIInventoryCellItem		(CInventoryItem* item);
											CUIInventoryCellItem		(shared_str section, Frect* rect = nullptr);
	virtual		bool						EqualTo						(CUICellItem* itm);
	virtual		void						UpdateItemText				();
				CUIDragItem*				CreateDragItem				();
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
								 CUIAmmoCellItem			(shared_str const& section);

				u32				 CalculateAmmoCount			(bool recursive);
				CWeaponAmmo*	 object						() { return smart_cast<CWeaponAmmo*>((CInventoryItem*)m_pData); }
};

class CUIAddonOwnerCellItem : public CUIInventoryCellItem
{
private:
	typedef CUIInventoryCellItem		inherited;

	struct SUIAddonSlot
	{
		SUIAddonSlot(xptr<CAddonSlot> const& slot);

		shared_str						name;
		shared_str						type;
		Fvector2						icon_offset;
		float							icon_step;
		bool							icon_background_draw;
		bool							icon_foreground_draw;
		
		shared_str						addon_section							= 0;
		u8								addon_type								= 0;
		u8								addon_index								= 0;

		xptr<CUIStatic> pAddonIcon{ nullptr };
	};

	typedef xr_vector<xptr<SUIAddonSlot>> VUISlots;

public:
										CUIAddonOwnerCellItem					(MAddonOwner* ao);
										CUIAddonOwnerCellItem					(shared_str CR$ section);

private:
	VUISlots							m_slots									= {};
	
	void								calculate_grid							(shared_str CR$ sect);
	void								process_slots							(VSlots CR$ slots, Fvector2 CR$ forwarded_offset);
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

class CUIArtefactModuleCellItem : public CUIInventoryCellItem
{
	typedef CUIInventoryCellItem super_;

public:
	CUIArtefactModuleCellItem(MArtefactModule* pItem);
	CUIArtefactModuleCellItem(shared_str const& strSection) : CUIInventoryCellItem(strSection) {}

public:
	bool EqualTo(CUICellItem* pCellItem) override;
	void UpdateItemText() override;

public:
	void setMode(float fMode);

private:
	MArtefactModule* const m_pItem{ nullptr };

	bool m_bShowMode{ false };
};

class CBuyItemCustomDrawCell :public ICustomDrawCellItem
{
	CGameFont*			m_pFont;
	string16			m_string;
public:
						CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont);
	virtual void		OnDraw					(CUICellItem* cell);

};
