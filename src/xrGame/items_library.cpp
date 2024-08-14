#include "stdafx.h"
#include "items_library.h"

CItemsLibrary::DATA CItemsLibrary::s_data;

bool CItemsLibrary::checkSection(shared_str CR$ section)
{
	return (READ_IF_EXISTS(pSettings, r_float, section, "inv_weight", 0.f) &&
		pSettings->line_exist(section, "category") &&
		!pSettings->line_exist(section, "pseudosection"))
		? true : false;
}

void CItemsLibrary::loadStaticData()
{
	s_data.clear						();
	for (auto section : pSettings->sections())
	{
		if (checkSection(section->Name))
		{
			auto& category				= s_data[pSettings->r_string(section->Name, "category")];
			auto& subcategory			= category[pSettings->r_string(section->Name, "subcategory")];
			auto& division				= subcategory[pSettings->r_string(section->Name, "division")];
			division.push_back			(section->Name);
		}
	}
}

CItemsLibrary::CATEGORY CR$ CItemsLibrary::getCategory(shared_str CR$ category)
{
	return								s_data.at(category.c_str());
}

CItemsLibrary::SUBCATEGORY CR$ CItemsLibrary::getSubcategory(shared_str CR$ category, shared_str CR$ subcategory)
{
	return								getCategory(category).at(subcategory.c_str());
}

CItemsLibrary::DIVISION CR$ CItemsLibrary::getDivision(shared_str CR$ category, shared_str CR$ subcategory, shared_str CR$ division)
{
	return								getSubcategory(category, subcategory).at(division.c_str());
}
