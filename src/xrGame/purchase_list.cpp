////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.cpp
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "purchase_list.h"
#include "ai\trader\ai_trader.h"

#include "xmod\items_library.h"

void CPurchaseList::process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner)
{
	owner.sell_useless_items		();
	CAI_Trader& trader				= smart_cast<CAI_Trader&>(owner);
	trader.supplies_list.clear_not_free();
	for (auto& item : ini_file.r_section(section).Data)
	{
		if (g_items_library->CheckSection(item.first))
		{
			GiveObject				(owner, item.first);
			continue;
		}

		string256					tmp0, tmp1, tmp2;
		shared_str category			= _GetItem(*item.first, 0, tmp0, '.');
		int count					= _GetItemCount(*item.first, '.');
		shared_str subcategory		= (count > 1) ? _GetItem(*item.first, 1, tmp1, '.') : "any";
		shared_str division			= (count > 2) ? _GetItem(*item.first, 2, tmp2, '.') : "any";

		for (auto& subcat : g_items_library->Get(category))
		{
			if (subcategory != "any" && subcat.first != subcategory)
				continue;
			for (auto& div : subcat.second)
			{
				if (division != "any" && div.first != division)
					continue;
				for (auto& sec : div.second)
					GiveObject		(owner, sec);
			}
		}
	}

	owner.inventory().InvalidateState();
}

void CPurchaseList::GiveObject(CInventoryOwner& owner, const shared_str& section)
{
	CAI_Trader& trader					= smart_cast<CAI_Trader&>(owner);
	trader.supplies_list.push_back		(section);
}
