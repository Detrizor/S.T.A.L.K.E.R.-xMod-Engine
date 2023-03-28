#pragma once
#include "inventory_item_object.h"

class CAttachment
{
private:
	CInventoryItemObject*	m_object;

public:
							CAttachment				();
							~CAttachment			()										{}
	virtual DLL_Pure*		_construct				();
	
			void			Load					(LPCSTR section);

protected:
			shared_str		m_boneName;			//позици€ кости котора€ будет использоватьс€ дл€ аттача, если пусто то аттачим к кости wpn_body
			Fvector			m_offset[2];
			Fmatrix			m_renderPos;		//“екуща€ позици€ дл€ рендеринга

public:
			void			UpdateRenderPos			(IRenderVisual* model, Fmatrix parent);
			void			Update_And_Render_World	(IRenderVisual* model, Fmatrix parent);
			void			Render					();
};

class CAttachmentObject : public CInventoryItemObject,
	public CAttachment
{
private:
	typedef	CInventoryItemObject					inherited;

public:
							CAttachmentObject		()										{}
	virtual					~CAttachmentObject		()										{}
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);
};
