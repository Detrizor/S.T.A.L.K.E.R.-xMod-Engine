#include "stdafx.h"

#include "items_library.h"

CItemsLibrary* g_items_library = NULL;

CItemsLibrary::CItemsLibrary()
{
	data.clear ();
	shared_str str;
	for (auto I : pSettings->sections())
	{
		if (!CheckSection(I->Name))
			continue;
		
		str									= pSettings->r_string(I->Name, "category");
		auto Category						= data.find(str);
		if (Category == data.end())
		{
			CATEGORY						cat;
			cat.clear						();
			data.insert						(std::make_pair(str, cat));
			Category						= data.find(str);
		}
		
		str									= pSettings->r_string(I->Name, "subcategory");
		auto Subcategory					= Category->second.find(str);
		if (Subcategory == Category->second.end())
		{
			SUBCATEGORY						subcat;
			subcat.clear					();
			Category->second.insert			(std::make_pair(str, subcat));
			Subcategory						= Category->second.find(str);
		}

		str									= pSettings->r_string(I->Name, "division");
		auto Division						= Subcategory->second.find(str);
		if (Division == Subcategory->second.end())
		{
			DIVISION						div;
			div.clear						();
			Subcategory->second.insert		(std::make_pair(str, div));
			Division						= Subcategory->second.find(str);
		}
		Division->second.push_back			(I->Name);
	}
}

const CATEGORY CItemsLibrary::Get(shared_str const& category) const
{
	return					data.find(category)->second;
}

const SUBCATEGORY CItemsLibrary::Get(shared_str const& category, shared_str const& subcategory) const
{
	CATEGORY cat			= Get(category);
	return					cat.find(subcategory)->second;
}

const DIVISION CItemsLibrary::Get(shared_str const& category, shared_str const& subcategory, shared_str const& division) const
{
	SUBCATEGORY subcat		= Get(category, subcategory);
	return					subcat.find(division)->second;
}

bool CItemsLibrary::CheckSection(shared_str const& section) const
{
	return pSettings->line_exist(section, "inv_name") && !pSettings->line_exist(section, "pseudosection");
}
