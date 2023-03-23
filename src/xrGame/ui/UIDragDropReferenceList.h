#pragma once
#include "UIDragDropListEx.h"
#include "../../xrServerEntities/inventory_space.h"
class CInventoryOwner;

class CUIDragDropReferenceList : public CUIDragDropListEx
{
private:
	typedef CUIDragDropListEx				inherited;
	typedef xr_vector<CUIStatic*>			ITEMS_REFERENCES_VEC;
	typedef ITEMS_REFERENCES_VEC::iterator	ITEMS_REFERENCES_VEC_IT;
	ITEMS_REFERENCES_VEC					m_references;
public:
	CUIDragDropReferenceList				();
	~CUIDragDropReferenceList				();

	void Initialize							();
	void ReloadReferences					(CInventoryOwner* pActor, u8 idx);
	void SetReference						(LPCSTR section, Ivector2 cell_pos, bool absent = false);

	virtual void __stdcall	OnItemDBClick	(CUIWindow* w, void* pData);
};