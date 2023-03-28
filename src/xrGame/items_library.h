#pragma once
#include "associative_vector.h"

typedef xr_vector<shared_str>							DIVISION;
typedef associative_vector<shared_str, DIVISION>		SUBCATEGORY;
typedef associative_vector<shared_str, SUBCATEGORY>		CATEGORY;
typedef associative_vector<shared_str, CATEGORY>		DATA;

class CItemsLibrary
{
private:
	DATA			data;

public:
					CItemsLibrary		();
					~CItemsLibrary		() {}

public:
	const CATEGORY		Get				(shared_str const& category) const;
	const SUBCATEGORY	Get				(shared_str const& category, shared_str const& subcategory) const;
	const DIVISION		Get				(shared_str const& category, shared_str const& subcategory, shared_str const& division) const;

	bool				CheckSection	(shared_str const& section) const;
};

extern CItemsLibrary* g_items_library;
