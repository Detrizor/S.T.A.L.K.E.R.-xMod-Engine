#pragma once

class CItemsLibrary
{
	typedef xr_vector<shared_str>					DIVISION;
	typedef xr_umap<_STD string, DIVISION>			SUBCATEGORY;
	typedef xr_umap<_STD string, SUBCATEGORY>		CATEGORY;
	typedef xr_umap<_STD string, CATEGORY>			DATA;

private:
	static DATA							s_data;

public:
	static void							loadStaticData							();
	static bool							validSection							(shared_str CR$ section);
	static CATEGORY CR$					getCategory								(LPCSTR category);
	static SUBCATEGORY CR$				getSubcategory							(LPCSTR category, LPCSTR subcategory);
	static DIVISION CR$					getDivision								(LPCSTR category, LPCSTR subcategory, LPCSTR division);
};
