#include "stdafx.h"
#include "UIDragDropReferenceList.h"
#include "UICellItem.h"
#include "UIStatic.h"
#include "../inventory.h"
#include "../inventoryOwner.h"
#include "../actor.h"
#include "../actor_defs.h"
#include "UIInventoryUtilities.h"
#include "../../xrEngine/xr_input.h"
#include "../UICursor.h"

CUIDragDropReferenceList::CUIDragDropReferenceList()
{
	AddCallbackStr("cell_item_reference", WINDOW_LBUTTON_DB_CLICK, CUIWndCallback::void_function(this, &CUIDragDropReferenceList::OnItemDBClick));
}

CUIDragDropReferenceList::~CUIDragDropReferenceList()
{
}

void CUIDragDropReferenceList::Initialize()
{
	int x = m_container->CellSize().x;
	int y = m_container->CellSize().y;
	int dx = m_container->CellsSpacing().x;
	int dy = m_container->CellsSpacing().y;
	for (int j = 0, j_e = m_container->CellsCapacity().y; j < j_e; j++)
	{
		for (int i = 0, i_e = m_container->CellsCapacity().x; i < i_e; i++)
		{
			m_references.push_back(xr_new<CUIStatic>());
			Fvector2 pos = Fvector2().set((x + dx) * i, (y + dy) * j);
			m_references.back()->SetAutoDelete(true);
			m_references.back()->SetWndPos(pos);
			m_references.back()->SetWndSize(Fvector2().set(x, y));
			AttachChild(m_references.back());
			m_references.back()->SetWindowName("cell_item_reference");
			Register(m_references.back());
		}
	}
}

void CUIDragDropReferenceList::ReloadReferences(CInventoryOwner* pActor, u8 idx)
{
	if (!pActor)
		return;

	LPCSTR item_name		= ACTOR_DEFS::g_quick_use_slots[idx];
	if (item_name && xr_strlen(item_name))
	{
		PIItem item			= pActor->inventory().Get(item_name, idx, true);
		SetReference		(item_name, Ivector2().set(0, 0), !item);
	}
	else
		SetReference		(NULL, Ivector2().set(0, 0));
}

void CUIDragDropReferenceList::SetReference(LPCSTR section, Ivector2 cell_pos, bool absent)
{
	int i					= cell_pos.x, j = cell_pos.y;
	CUIStatic* ref			= m_references[i + j * CellsCapacity().x];
	if (section == NULL)
	{
		ref->TextureOff		();
		return;
	}

	Frect					texture_rect;
	CInventoryItem::readIcon(texture_rect, section);
	
	float r_x		= texture_rect.width();
	float r_y		= texture_rect.height();

	float x			= (float)m_container->CellSize().x, y = (float)m_container->CellSize().y;
	float dx		= (float)m_container->CellsSpacing().x, dy = (float)m_container->CellsSpacing().y;
	float scale_x	= x / r_x, scale_y = y / r_y;
	float scale		= _min(scale_x, scale_y);
	r_x				*= scale;
	r_y				*= scale;
	Fvector2 pos	= (r_x < r_y) ? Fvector2().set((x + dx) * i + (x - r_x) / 2, (y + dy) * j) : Fvector2().set((x + dx) * i, (y + dy) * j + (y - r_y) / 2);
	Fvector2 size	= Fvector2().set(r_x, r_y);

	ref->SetWndPos				(pos);
	ref->SetWndSize				(size);
	ref->SetShader				(InventoryUtilities::GetEquipmentIconsShader(section));
	ref->SetTextureRect			(texture_rect);
	ref->TextureOn				();
	ref->SetStretchTexture		(true);
	ref->SetTextureColor		(color_rgba(255, 255, 255, (absent) ? 100 : 255));
}

void CUIDragDropReferenceList::OnItemDBClick(CUIWindow* w, void* pData)
{
	CUIStatic* ref = smart_cast<CUIStatic*>(w);
	ITEMS_REFERENCES_VEC_IT it = std::find(m_references.begin(), m_references.end(), ref);
	if (it != m_references.end())
	{
		u8 index = u8(it - m_references.begin());
		xr_strcpy(ACTOR_DEFS::g_quick_use_slots[index], "");
		u8 width = (u8)CellsCapacity().x;
		SetReference(NULL, Ivector2().set(index % width, index / width));
	}
}