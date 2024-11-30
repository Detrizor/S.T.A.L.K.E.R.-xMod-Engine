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
#include "ui/UIInventoryUtilities.h"

#include "items_library.h"

void CPurchaseList::process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner)
{
	bool ammo							= false;
	auto add_item = [&owner, &ammo](shared_str CR$ sec)
	{
		owner.suppliesList.push_back	(sec);
		if (!ammo && ItemCategory(sec, "ammo"))
			ammo						= true;
	};

	owner.sell_useless_items			();
	owner.suppliesList.clear			();
	for (auto& item : ini_file.r_section(section).Data)
	{
		if (CItemsLibrary::validSection(item.first))
		{
			add_item					(item.first);
			continue;
		}

		string256						tmp0, tmp1, tmp2;
		LPCSTR category					= _GetItem(*item.first, 0, tmp0, '.');
		int count						= _GetItemCount(*item.first, '.');
		_STD string subcategory			= (count > 1) ? _GetItem(*item.first, 1, tmp1, '.') : "any";
		_STD string division			= (count > 2) ? _GetItem(*item.first, 2, tmp2, '.') : "any";

		for (auto& subcat : CItemsLibrary::getCategory(category))
		{
			if (subcategory != "any" && subcat.first != subcategory)
				continue;
			for (auto& div : subcat.second)
			{
				if (division != "any" && div.first != division)
					continue;
				for (auto& sec : div.second)
					add_item			(sec);
			}
		}
	}

	if (!ammo)
	{
		owner.suppliesList.sort([](auto CR$ s1, auto CR$ s2)
			{
				auto c1					= CInventoryItem::readBaseCost(s1.c_str(), true);
				auto c2					= CInventoryItem::readBaseCost(s2.c_str(), true);
				return					(c1 < c2);
			}
		);
	}

	owner.inventory().InvalidateState	();
}
