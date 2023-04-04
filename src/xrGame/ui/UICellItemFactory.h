#pragma once

class CUICellItem;
class CInventoryItem;

CUICellItem*	create_cell_item(CInventoryItem* itm);
CUICellItem*	create_cell_item_from_section(shared_str& section);
