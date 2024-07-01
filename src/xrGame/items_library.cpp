#include "stdafx.h"
#include "items_library.h"

CItemsLibrary* g_items_library = nullptr;

bool CItemsLibrary::checkSection(shared_str CR$ section)
{
	return								READ_IF_EXISTS(pSettings, r_float, section, "inv_weight", 0.f) &&
		pSettings->line_exist(section, "category") && !pSettings->line_exist(section, "pseudosection");
}

CItemsLibrary::CItemsLibrary()
{
	shared_str							str;
	for (auto I : pSettings->sections())
	{
		if (!checkSection(I->Name))
			continue;
		
		str								= pSettings->r_string(I->Name, "category");
		auto Category					= m_data.find(str);
		if (Category == m_data.end())
		{
			CATEGORY					cat;
			cat.clear					();
			m_data.insert				(std::make_pair(str, cat));
			Category					= m_data.find(str);
		}
		
		str								= pSettings->r_string(I->Name, "subcategory");
		auto Subcategory				= Category->second.find(str);
		if (Subcategory == Category->second.end())
		{
			SUBCATEGORY					subcat;
			subcat.clear				();
			Category->second.insert		(std::make_pair(str, subcat));
			Subcategory					= Category->second.find(str);
		}

		str								= pSettings->r_string(I->Name, "division");
		auto Division					= Subcategory->second.find(str);
		if (Division == Subcategory->second.end())
		{
			DIVISION					div;
			div.clear					();
			Subcategory->second.insert	(std::make_pair(str, div));
			Division					= Subcategory->second.find(str);
		}
		Division->second.push_back		(I->Name);
	}
}

CItemsLibrary::CATEGORY CR$ CItemsLibrary::getCategory(shared_str CR$ category) const
{
	return								m_data.find(category)->second;
}

CItemsLibrary::SUBCATEGORY CR$ CItemsLibrary::getSubcategory(shared_str CR$ category, shared_str CR$ subcategory) const
{
	return								getCategory(category).find(subcategory)->second;
}

CItemsLibrary::DIVISION CR$ CItemsLibrary::getDivision(shared_str CR$ category, shared_str CR$ subcategory, shared_str CR$ division) const
{
	return								getSubcategory(category, subcategory).find(division)->second;
}
