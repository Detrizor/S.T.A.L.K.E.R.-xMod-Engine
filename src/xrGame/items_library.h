#pragma once
#include "associative_vector.h"

class CItemsLibrary
{
	typedef xr_vector<shared_str>				DIVISION;
	typedef xr_umap<LPCSTR, DIVISION>			SUBCATEGORY;
	typedef xr_umap<LPCSTR, SUBCATEGORY>		CATEGORY;
	typedef xr_umap<LPCSTR, CATEGORY>			DATA;

private:
	static DATA							s_data;

public:
	static void							loadStaticData							();
	static bool							checkSection							(shared_str CR$ section);
	static CATEGORY CR$					getCategory								(shared_str CR$ category);
	static SUBCATEGORY CR$				getSubcategory							(shared_str CR$ category, shared_str CR$ subcategory);
	static DIVISION CR$					getDivision								(shared_str CR$ category, shared_str CR$ subcategory, shared_str CR$ division);
};
