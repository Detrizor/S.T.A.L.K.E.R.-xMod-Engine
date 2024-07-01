#pragma once
#include "associative_vector.h"

class CItemsLibrary
{
	typedef xr_vector<shared_str>							DIVISION;
	typedef associative_vector<shared_str, DIVISION>		SUBCATEGORY;
	typedef associative_vector<shared_str, SUBCATEGORY>		CATEGORY;
	typedef associative_vector<shared_str, CATEGORY>		DATA;

private:
	DATA								m_data									= {};

public:
										CItemsLibrary							();

public:
	static bool							checkSection							(shared_str CR$ section);
	
	CATEGORY CR$						getCategory							C$	(shared_str CR$ category);
	SUBCATEGORY CR$						getSubcategory						C$	(shared_str CR$ category, shared_str CR$ subcategory);
	DIVISION CR$						getDivision							C$	(shared_str CR$ category, shared_str CR$ subcategory, shared_str CR$ division);
};

extern CItemsLibrary* g_items_library;
