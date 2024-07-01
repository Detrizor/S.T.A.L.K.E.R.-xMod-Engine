#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"
#include "addon_owner.h"

CUICellItem*	create_cell_item(CInventoryItem* itm)
{
	if (auto ammo = itm->cast<CWeaponAmmo*>())
		return							xr_new<CUIAmmoCellItem>(ammo);

	if (auto ao = itm->cast<CAddonOwner*>())
		return							xr_new<CUIAddonOwnerCellItem>(ao);

	return								xr_new<CUIInventoryCellItem>(itm);
}

CUICellItem* create_cell_item_from_section(shared_str CR$ section)
{
	if (READ_IF_EXISTS(pSettings, r_bool, section, "addon_owner", FALSE))
		return							xr_new<CUIAddonOwnerCellItem>(section);

	return								xr_new<CUIInventoryCellItem>(section);
}
