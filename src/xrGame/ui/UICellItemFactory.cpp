#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"
#include "..\addon_owner.h"

CUICellItem*	create_cell_item(CInventoryItem* itm)
{
	VERIFY								(itm);
	CUICellItem*						cell_item;

	CWeaponAmmo* pAmmo					= itm->cast<CWeaponAmmo*>();
	CAddonOwner* ao						= itm->cast<CAddonOwner*>();
	if (pAmmo)
		cell_item						= xr_new<CUIAmmoCellItem>(pAmmo);
	else if (ao)
		cell_item						= xr_new<CUIAddonOwnerCellItem>(ao);
	else
		cell_item						= xr_new<CUIInventoryCellItem>(itm);

	return								cell_item;
}

CUICellItem* create_cell_item_from_section(shared_str& section)
{
	if (READ_IF_EXISTS(pSettings, r_string, section, "slots", 0))
		return							xr_new<CUIAddonOwnerCellItem>(section);
	return								xr_new<CUIInventoryCellItem>(section);
}
