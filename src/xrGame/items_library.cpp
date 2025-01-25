#include "stdafx.h"
#include "items_library.h"
#include "string_table.h"

CItemsLibrary::DATA CItemsLibrary::s_data;

bool CItemsLibrary::validSection(shared_str CR$ section)
{
	if (pSettings->r_bool_ex(section, "pseudosection", false))
		return							false;

	if (!pSettings->r_float_ex(section, "inv_weight", 0.f) && !pSettings->r_float_ex(section, "net_weight", 0.f))
		return							false;

	return								pSettings->r_string_ex(section, "inv_name", NULL) || CStringTable().exists(shared_str().printf("st_%s_name", section.c_str()));
}

void CItemsLibrary::loadStaticData()
{
	s_data.clear						();
	for (auto section : pSettings->sections())
	{
		if (validSection(section->Name))
		{
			auto& category				= s_data[pSettings->r_string(section->Name, "category")];
			auto& subcategory			= category[pSettings->r_string(section->Name, "subcategory")];
			auto& division				= subcategory[pSettings->r_string(section->Name, "division")];
			division.push_back			(section->Name);
		}
	}
}

CItemsLibrary::CATEGORY CR$ CItemsLibrary::getCategory(LPCSTR category)
{
	return								s_data.at(category);
}

CItemsLibrary::SUBCATEGORY CR$ CItemsLibrary::getSubcategory(LPCSTR category, LPCSTR subcategory)
{
	return								getCategory(category).at(subcategory);
}

CItemsLibrary::DIVISION CR$ CItemsLibrary::getDivision(LPCSTR category, LPCSTR subcategory, LPCSTR division)
{
	return								getSubcategory(category, subcategory).at(division);
}
