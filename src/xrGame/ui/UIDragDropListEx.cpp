#include "stdafx.h"
#include "UIDragDropListEx.h"
#include "UIScrollBar.h"
#include "object_broker.h"
#include "UICellItem.h"
#include "UICursor.h"
#include "../Level.h"
//Alundaio
#include "../Inventory.h"
#include <dinput.h>
//-Alundaio
#include "../UIGameCustom.h"
#include "UIActorMenu.h"

CUIDragItem* CUIDragDropListEx::m_drag_item = NULL;

constexpr bool grid_render = true;
constexpr bool grid_render_empty = false;

void CUICell::Clear()
{
	m_bMainItem = false;
	if(m_item)	m_item->SetOwnerList(NULL);
	m_item		= NULL; 
}

CUIDragDropListEx::CUIDragDropListEx()
{
	m_flags.zero				();
	m_container					= xr_new<CUICellContainer>(this);
	m_vScrollBar				= xr_new<CUIScrollBar>();
	m_vScrollBar->SetAutoDelete	(true);
	m_selected_item				= NULL;
	m_bConditionProgBarVisible	= false;

	SetCellSize					(50.f, sAbsolute);
	SetCellsCapacity			(Ivector2().set(0,0));

	AttachChild					(m_container);
	AttachChild					(m_vScrollBar);

	m_vScrollBar->SetWindowName	("scroll_v");
	Register					(m_vScrollBar);
	AddCallbackStr				("scroll_v",	SCROLLBAR_VSCROLL,				CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnScrollV)		);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_DRAG,			CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemStartDragging)	);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_DROP,			CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemDrop)			);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_SELECTED,		CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemSelected)			);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_LBUTTON_CLICK,	CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemLButtonClick)			);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_RBUTTON_CLICK,	CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemRButtonClick)			);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_DB_CLICK,		CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemDBClick)			);
	AddCallbackStr				("cell_item",	DRAG_DROP_ITEM_FOCUSED_UPDATE,	CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemFocusedUpdate)			);
	AddCallbackStr				("cell_item",	WINDOW_FOCUS_RECEIVED,			CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemFocusReceived)			);
	AddCallbackStr				("cell_item",	WINDOW_FOCUS_LOST,				CUIWndCallback::void_function		(this, &CUIDragDropListEx::OnItemFocusLost)			);

	back_color = 0xFFFFFFFF;
}

CUIDragDropListEx::~CUIDragDropListEx()
{
	DestroyDragItem		();

	delete_data					(m_container);
}

void CUIDragDropListEx::SetAutoGrow(bool b)						
{
	m_flags.set(flAutoGrow,b);
}
bool CUIDragDropListEx::IsAutoGrow()								
{
	return !!m_flags.test(flAutoGrow);
}
void CUIDragDropListEx::SetGrouping(bool b)						
{
	m_flags.set(flGroupSimilar,b);
}
bool CUIDragDropListEx::IsGrouping()
{
	return !!m_flags.test(flGroupSimilar);
}
void CUIDragDropListEx::SetCustomPlacement(bool b)
{
	m_flags.set(flCustomPlacement,b);
}

bool CUIDragDropListEx::GetCustomPlacement()
{
	return !!m_flags.test(flCustomPlacement);
}
void CUIDragDropListEx::SetVerticalPlacement(bool b)
{
	m_flags.set(flVerticalPlacement,b);
}

void CUIDragDropListEx::SetAlwaysShowScroll(bool b)
{
	m_flags.set(flAlwaysShowScroll,b);
}

bool CUIDragDropListEx::GetVerticalPlacement()
{
	return !!m_flags.test(flVerticalPlacement);
}

void CUIDragDropListEx::SetVirtualCells(bool b)
{
	m_flags.set(flVirtualCells,b);
}

bool CUIDragDropListEx::GetVirtualCells()
{
	return !!m_flags.test(flVirtualCells);
}

void CUIDragDropListEx::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUIDragDropListEx::InitDragDropList()
{
	m_vScrollBar->InitScrollBar			(Fvector2().set(GetWndSize().x, 0.0f), GetWndSize().y, false);
	m_vScrollBar->SetWndPos				(Fvector2().set(m_vScrollBar->GetWndPos().x - m_vScrollBar->GetWidth(), m_vScrollBar->GetWndPos().y));
}

void CUIDragDropListEx::OnScrollV(CUIWindow* w, void* pData)
{
	m_container->SetWndPos		(Fvector2().set(m_container->GetWndPos().x, float(-m_vScrollBar->GetScrollPos())));
}

void CUIDragDropListEx::CreateDragItem(CUICellItem* itm)
{
	R_ASSERT							(!m_drag_item);
	m_drag_item							= itm->CreateDragItem();

	if ( m_drag_item )
	{
		GetParent()->SetCapture			(m_drag_item, true);
	}
}

void CUIDragDropListEx::DestroyDragItem()
{
	if (m_selected_item && m_drag_item && m_drag_item->ParentItem() == m_selected_item)
	{
		VERIFY							(GetParent()->GetMouseCapturer() == m_drag_item);
		GetParent()->SetCapture			(nullptr, false);

		delete_data						(m_drag_item);
	}
}

Fvector2 CUIDragDropListEx::GetDragItemPosition()
{	//Alun: More accurate then Left-Top of dragged icon
	return								GetUICursor().GetCursorPosition();//m_drag_item->GetPosition();
}

void CUIDragDropListEx::OnDragEvent(CUIDragItem* drag_item, bool b_receive)
{
	if (m_f_drag_event)
		m_f_drag_event					(drag_item, b_receive);
}

void CUIDragDropListEx::OnItemStartDragging(CUIWindow* w, void* pData)
{
	CUICellItem* itm					= smart_cast<CUICellItem*>(w);
	if (itm != m_selected_item)
		OnItemSelected					(w, pData);
	if (!m_f_item_start_drag || !m_f_item_start_drag(itm))
		CreateDragItem					(itm);
}

void CUIDragDropListEx::OnItemDrop(CUIWindow* w, void* pData)
{
	CUICellItem* itm					= smart_cast<CUICellItem*>(w);
	if (itm != m_selected_item)
		OnItemSelected(w, pData);
	VERIFY								(itm->OwnerList() == itm->OwnerList());

	if (m_f_item_drop && m_f_item_drop(itm))
	{
		DestroyDragItem					();
		return;
	}

	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= m_drag_item->BackList();
	if (old_owner && new_owner && (old_owner != new_owner || GetCustomPlacement()))
	{
		CUICellItem* i					= old_owner->RemoveItem(itm, (old_owner == new_owner) );
		if (new_owner->CanSetItem(i))
		{
			while(i->ChildsCount())
			{
				CUICellItem* _chld		= i->PopChild(NULL);
				new_owner->SetItem		(_chld, old_owner->GetDragItemPosition());
			}
			new_owner->SetItem			(i,old_owner->GetDragItemPosition());
		}
	}
	DestroyDragItem						();
}

void CUIDragDropListEx::OnItemDBClick(CUIWindow* w, void* pData)
{
	CUICellItem* itm					= smart_cast<CUICellItem*>(w);
	if (itm != m_selected_item)
		OnItemSelected					(w, pData);

	if (m_f_item_db_click)
	{
		if (Level().IR_GetKeyState(DIK_LCONTROL))
			for (size_t i = 0, cnt = itm->ChildsCount(); i < cnt; ++i)
				m_f_item_db_click		(itm->Child(i));

		if (m_f_item_db_click(itm))
		{
			DestroyDragItem				();
			return;
		}
	}

	CUIDragDropListEx* old_owner		= itm->OwnerList();
	VERIFY								(!m_drag_item);
	VERIFY								(old_owner == this);

	if (old_owner && old_owner->GetCustomPlacement())
	{
		CUICellItem* i					= old_owner->RemoveItem(itm, true);
		old_owner->SetItem				(i);
	}

	DestroyDragItem						();
}

void CUIDragDropListEx::OnItemSelected(CUIWindow* w, void* pData)
{
	m_selected_item						= smart_cast<CUICellItem*>(w);
	VERIFY								(m_selected_item);
	if(m_f_item_selected)
		m_f_item_selected				(m_selected_item);
}

void CUIDragDropListEx::OnItemFocusReceived(CUIWindow* w, void* pData)
{
	if (m_f_item_focus_received)
	{
		CUICellItem* itm				= smart_cast<CUICellItem*>(w);
		m_f_item_focus_received			(itm);
	}
}

void  CUIDragDropListEx::OnItemFocusLost(CUIWindow* w, void* pData)
{
	if (m_f_item_focus_lost)
	{
		CUICellItem* itm				= smart_cast<CUICellItem*>(w);
		m_f_item_focus_lost				(itm);
	}
}

void CUIDragDropListEx::OnItemFocusedUpdate(CUIWindow* w, void* pData)
{
	if (m_f_item_focused_update)
	{
		CUICellItem* itm				= smart_cast<CUICellItem*>(w);
		m_f_item_focused_update			(itm);
	}
}

void CUIDragDropListEx::OnItemRButtonClick(CUIWindow* w, void* pData)
{
	CUICellItem* itm					= smart_cast<CUICellItem*>(w);
	if (m_f_item_rbutton_click) 
		m_f_item_rbutton_click			(itm);
}

void CUIDragDropListEx::OnItemLButtonClick(CUIWindow* w, void* pData)
{
	CUICellItem* itm					= smart_cast<CUICellItem*>(w);
	if (m_f_item_lbutton_click) 
		m_f_item_lbutton_click			(itm);
}

void CUIDragDropListEx::GetClientArea(Frect& r)
{
	GetAbsoluteRect						(r);
	if (m_vScrollBar->GetVisible() || m_flags.test(flAlwaysShowScroll))
		r.x2							-= m_vScrollBar->GetWidth();
}

void CUIDragDropListEx::ClearAll(bool bDestroy)
{
	DestroyDragItem						();
	m_container->ClearAll				(bDestroy);
	m_selected_item						= nullptr;
	m_container->SetWndPos				(vZero2);
	ResetCellsCapacity					();
}

void CUIDragDropListEx::Compact()
{
	CUIWindow::WINDOW_LIST wl			= m_container->GetChildWndList();
	ClearAll							(false);
	for (auto& wnd : wl)
		SetItem							(static_cast<CUICellItem*>(wnd));
}

void CUIDragDropListEx::Draw()
{
	inherited::Draw				();

	if(0 && bDebug){
		CGameFont* F		= UI().Font().pFontDI;
		F->SetAligment		(CGameFont::alCenter);
		F->SetHeightI		(0.02f);
		F->OutSetI			(0.f,-0.5f);
		F->SetColor			(0xffffffff);
		Ivector2			pt = m_container->PickCell(GetUICursor().GetCursorPosition());
		F->OutNext			("%d-%d",pt.x, pt.y);
	};

}

void CUIDragDropListEx::Update()
{
	inherited::Update			();

	if( m_drag_item ){
		Frect	wndRect;
		GetAbsoluteRect(wndRect);
		Fvector2 cp			= GetUICursor().GetCursorPosition();
		if(wndRect.in(cp)){
			if(NULL==m_drag_item->BackList())
				m_drag_item->SetBackList(this);
		}else
			if( this==m_drag_item->BackList() )
				m_drag_item->SetBackList(NULL);
	}
}

void CUIDragDropListEx::ReinitScroll()
{
		float h1 = m_container->GetWndSize().y;
		float h2 = GetWndSize().y;
		VERIFY						(_valid(h1));
		VERIFY						(_valid(h2));
		float dh = h1-h2;
		m_vScrollBar->Show			( (dh > 0) || m_flags.test(flAlwaysShowScroll) );
		m_vScrollBar->Enable		( (dh > 0) || m_flags.test(flAlwaysShowScroll) );

		if ( dh < 0 )
		{
//			dh = 0;
			m_vScrollBar->SetRange	(0, 0);
		}
		else
		{
			m_vScrollBar->SetRange	(0, iFloor(dh));
		}
		m_vScrollBar->SetScrollPos	(0);
		m_vScrollBar->SetStepSize	(CellSize().y/3);
//		m_vScrollBar->SetPageSize	(iFloor(GetWndSize().y/float(CellSize().y)));
		m_vScrollBar->SetPageSize	( 1/*CellSize().y*/ );

		m_container->SetWndPos		(Fvector2().set(0,0));
}

bool CUIDragDropListEx::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	bool b = inherited::OnMouseAction		(x,y,mouse_action);

	if(m_vScrollBar->IsShown())
	{
		switch(mouse_action){
		case WINDOW_MOUSE_WHEEL_DOWN:
			for( u8 i = 0; i < 4; ++i )		{	m_vScrollBar->TryScrollInc();	}
			return true;
			break;

		case WINDOW_MOUSE_WHEEL_UP:
			for( u8 i = 0; i < 4; ++i )		{	m_vScrollBar->TryScrollDec();	}
			return true;
			break;
		}
	}
	return b;
}

const Ivector2& CUIDragDropListEx::CellsCapacity()
{
	return m_container->CellsCapacity();
}

void CUIDragDropListEx::SetCellsCapacity(const Ivector2 c)
{
	m_container->SetCellsCapacity(c);
}

const Ivector2& CUIDragDropListEx::CellSize()
{
	return m_container->CellSize();
}
const Ivector2& CUIDragDropListEx::CellsSpacing()
{
	return m_container->CellsSpacing();
}

void CUIDragDropListEx::SetCellSize(const float& size, const EScaling& scaling)
{
	m_container->SetCellSize(size, scaling);
}

void CUIDragDropListEx::SetCellsSpacing(const float& size, const EScaling& scaling)
{
	m_container->SetCellsSpacing(size, scaling);
}

int CUIDragDropListEx::ScrollPos()
{
	return m_vScrollBar->GetScrollPos();
}

void CUIDragDropListEx::SetItem(CUICellItem* itm) //auto
{
	if (!m_container->AddSimilar(itm))
	{
		Ivector2 dest_cell_pos			= m_container->FindFreeCell(itm->GetGridSize());
		SetItem							(itm, dest_cell_pos);
		CurrentGameUI()->GetActorMenu().colorizeItem(itm);
		itm->m_select_equipped			= false;
	}
}

void CUIDragDropListEx::SetItem(CUICellItem* itm, Fvector2 abs_pos) // start at cursor pos
{
	if (!m_container->AddSimilar(itm))
	{
		Ivector2 dest_cell_pos			= m_container->PickCell(abs_pos);
		if (m_container->ValidCell(dest_cell_pos) && m_container->IsRoomFree(dest_cell_pos,itm->GetGridSize()))
			SetItem						(itm, dest_cell_pos);
		else
			SetItem						(itm);
	}
}

void CUIDragDropListEx::SetItem(CUICellItem* itm, Ivector2 cell_pos) // start at cell
{
	if (!m_container->AddSimilar(itm))
	{
		R_ASSERT						(m_container->IsRoomFree(cell_pos, itm->GetGridSize()));

		m_container->PlaceItemAtPos		(itm, cell_pos);

		itm->SetWindowName				("cell_item");
		Register						(itm);
		itm->SetOwnerList				(this);
	}
}

bool CUIDragDropListEx::CanSetItem(CUICellItem* itm)
{
	if (m_container->HasFreeSpace(itm->GetGridSize()))
		return							true;
	Compact								();
	return m_container->HasFreeSpace	(itm->GetGridSize());
}

CUICellItem* CUIDragDropListEx::RemoveItem(CUICellItem* itm, bool force_root)
{
	CUICellItem* i						= m_container->RemoveItem(itm, force_root);
	i->SetOwnerList						(nullptr);
	return								i;
}

u32 CUIDragDropListEx::ItemsCount()
{
	return								m_container->GetChildWndList().size();
}

bool CUIDragDropListEx::IsOwner(CUICellItem* itm)
{
	return								m_container->IsChild(itm);
}

CUICellItem* CUIDragDropListEx::GetItemIdx(u32 idx)
{
	R_ASSERT(idx<ItemsCount());
	WINDOW_LIST_it it = m_container->GetChildWndList().begin();
	std::advance	(it, idx);
	return smart_cast<CUICellItem*>(*it);
}

void CUIDragDropListEx::clear_select_armament()
{
	m_container->clear_select_armament();
}

void CUIDragDropListEx::SetCellsVertAlignment(xr_string alignment)
{
	if(strchr(alignment.c_str(), 't'))
	{
		m_virtual_cells_alignment.y = 0;
		return;
	}
	if(strchr(alignment.c_str(), 'b'))
	{
		m_virtual_cells_alignment.y = 2;
		return;
	}
	m_virtual_cells_alignment.y = 1;
}

void CUIDragDropListEx::SetCellsHorizAlignment(xr_string alignment)
{
	if(strchr(alignment.c_str(), 'l'))
	{
		m_virtual_cells_alignment.x = 0;
		return;
	}
	if(strchr(alignment.c_str(), 'r'))
	{
		m_virtual_cells_alignment.x = 2;
		return;
	}
	m_virtual_cells_alignment.x = 1;
}

Ivector2 CUIDragDropListEx::PickCell(const Fvector2& abs_pos) 
{
	return m_container->PickCell(abs_pos);
}

CUICell& CUIDragDropListEx::GetCellAt(const Ivector2& pos) 
{
	return m_container->GetCellAt(pos);
}

void CUIDragDropListEx::ResetCellsCapacity()
{
	SetCellsCapacity(Ivector2().set((GetWidth() - (GetVirtualCells() ? 0.f : m_vScrollBar->GetWidth())) / (float)CellSize().x, GetHeight() / (float)CellSize().y));
}

// =================================================================================================

CUICellContainer::CUICellContainer(CUIDragDropListEx* parent)
{
	m_pParentDragDropList		= parent;
	hShader->create				( "hud\\fog_of_war", "ui\\ui_grid" );
	m_cellSize					= 50.f;
	m_cellScaling				= sAbsolute;
	m_spacingSize				= 0.f;
	m_spacingScaling			= sAbsolute;
}

CUICellContainer::~CUICellContainer()
{
}

bool CUICellContainer::AddSimilar(CUICellItem* itm)
{
	if (!m_pParentDragDropList->IsGrouping())
		return			false;

	PIItem iitem		= (PIItem)itm->m_pData;
	if (iitem)
	{
		if (!iitem->CanStack())
			return		false;
		if (iitem->m_pInventory && iitem->m_pInventory->ItemFromSlot(iitem->BaseSlot()) == iitem)
			return		false;
	}

	CUICellItem* i		= FindSimilar(itm);
	if (i == NULL || i == itm || itm->ChildsCount() > 0)
		return			false;

	i->PushChild		(itm);
	itm->SetOwnerList	(m_pParentDragDropList);

	return				true;
}

CUICellItem* CUICellContainer::FindSimilar(CUICellItem* itm)
{
	for(WINDOW_LIST_it it = m_ChildWndList.begin(); m_ChildWndList.end()!=it; ++it)
	{
#ifdef DEBUG
		CUICellItem* i = smart_cast<CUICellItem*>(*it);
#else
		CUICellItem* i = (CUICellItem*)(*it);
#endif
		if (i == itm)
			continue;

		if (!i->EqualTo(itm))
			continue;

		//Alundaio: Don't stack equipped items
		PIItem	iitem = (PIItem)i->m_pData;
		if (iitem && iitem->m_pInventory)
		{
			if (iitem->m_pInventory->ItemFromSlot(iitem->BaseSlot()) == iitem)
				continue;

			if (!iitem->CanStack())
				continue;
		}
		//-Alundaio

		return i;
	}
	return NULL;
}

void CUICellContainer::PlaceItemAtPos(CUICellItem* itm, Ivector2& cell_pos)
{
	Ivector2 cs				= itm->GetGridSize();
	if(m_pParentDragDropList->GetVerticalPlacement())
		std::swap(cs.x,cs.y);

	for(int x=0; x<cs.x; ++x)
	{
		for(int y=0; y<cs.y; ++y)
		{
			CUICell& C		= GetCellAt(Ivector2().set(x,y).add(cell_pos));
			C.SetItem		(itm,(x==0&&y==0));
		}
	}
	itm->SetWndSize			( Fvector2().set( (CellSize().x*cs.x),		(CellSize().y*cs.y)		 )	);
	if(!m_pParentDragDropList->GetVirtualCells())
		itm->SetWndPos			( Fvector2().set( ((CellsSpacing().x+CellSize().x)*cell_pos.x), ((CellsSpacing().y+CellSize().y)*cell_pos.y))	);
	else
	{
		Ivector2 alignment_vec	= m_pParentDragDropList->GetVirtualCellsAlignment();
		Fvector2 pos = Fvector2().set(0,0);
		if(alignment_vec.x == 1)
			pos.x = (m_pParentDragDropList->GetWndSize().x-cs.x*(CellsSpacing().x+CellSize().x))/2;
		else if(alignment_vec.x == 2)
			pos.x = m_pParentDragDropList->GetWndSize().x-cs.x*(CellsSpacing().x+CellSize().x);
		
		if(alignment_vec.y == 1)
			pos.y = (m_pParentDragDropList->GetWndSize().y-cs.y*(CellsSpacing().y+CellSize().y))/2;
		else if(alignment_vec.y == 2)
			pos.y = m_pParentDragDropList->GetWndSize().y-cs.y*(CellsSpacing().y+CellSize().y);
		itm->SetWndPos(pos);
	}

	AttachChild				(itm);
	itm->OnAfterChild		(m_pParentDragDropList);
}

CUICellItem* CUICellContainer::RemoveItem(CUICellItem* itm, bool force_root)
{
	for(WINDOW_LIST_it it = m_ChildWndList.begin(); m_ChildWndList.end()!=it; ++it)
	{
		CUICellItem* i		= (CUICellItem*)(*it);
		
		if(i->HasChild(itm))
		{
			CUICellItem* iii	= i->PopChild(itm);
			R_ASSERT			(0==iii->ChildsCount());
			return				iii;
		}
	}

	if(!force_root && itm->ChildsCount())
	{
		CUICellItem* iii	=	itm->PopChild(NULL);
		R_ASSERT			(0==iii->ChildsCount());
		return				iii;
	}

	Ivector2 pos			= GetItemPos(itm);
	Ivector2 cs				= itm->GetGridSize();

	if(m_pParentDragDropList->GetVerticalPlacement())
		std::swap(cs.x,cs.y);

	for(int x=0; x<cs.x;++x)
		for(int y=0; y<cs.y;++y)
		{
			CUICell& C		= GetCellAt(Ivector2().set(x,y).add(pos));
			C.Clear			();
		}

	itm->SetOwnerList		(NULL);
	DetachChild				(itm);
	return					itm;
}

Ivector2 CUICellContainer::FindFreeCell	(const Ivector2& _size)
{
	Ivector2 tmp;
	Ivector2 size = _size;

	if(m_pParentDragDropList->GetVerticalPlacement())
		std::swap(size.x, size.y);

	for(tmp.y=0; tmp.y<=m_cellsCapacity.y-size.y; ++tmp.y )
		for(tmp.x=0; tmp.x<=m_cellsCapacity.x-size.x; ++tmp.x )
			if(IsRoomFree(tmp,_size))
				return  tmp;

	if(m_pParentDragDropList->IsAutoGrow())
	{
		Grow	();
		return							FindFreeCell	(size);
	}else{
		m_pParentDragDropList->Compact		();
		for(tmp.y=0; tmp.y<=m_cellsCapacity.y-size.y; ++tmp.y )
			for(tmp.x=0; tmp.x<=m_cellsCapacity.x-size.x; ++tmp.x )
				if(IsRoomFree(tmp,_size))
					return  tmp;

		R_ASSERT2		(0,"there are no free room to place item");
	}
	return			tmp;
}

bool CUICellContainer::HasFreeSpace		(const Ivector2& _size)
{
	Ivector2 tmp;
	Ivector2 size = _size;

	if(m_pParentDragDropList->GetVerticalPlacement())
		std::swap(size.x, size.y);

	for(tmp.y=0; tmp.y<=m_cellsCapacity.y-size.y; ++tmp.y )
		for(tmp.x=0; tmp.x<=m_cellsCapacity.x-size.x; ++tmp.x )
			if(IsRoomFree(tmp,_size))
				return true;

	return false;
}

bool CUICellContainer::IsRoomFree(const Ivector2& pos, const Ivector2& _size)
{
	Ivector2 tmp;

	Ivector2 size = _size;
	if(m_pParentDragDropList->GetVerticalPlacement())
		std::swap(size.x, size.y);

	for(tmp.x =pos.x; tmp.x<pos.x+size.x; ++tmp.x)
		for(tmp.y =pos.y; tmp.y<pos.y+size.y; ++tmp.y)
		{
			if(!ValidCell(tmp))		return		false;

			CUICell& C				= GetCellAt(tmp);

			if(!C.Empty())			return		false;
		}
	return true;
}

void CUICellContainer::GetTexUVLT(Fvector2& uv, u32 col, u32 row, u8 select_mode)
{
	switch ( select_mode )
	{
	case 0:		uv.set(0.00f, 0.0f);	break;
	case 1:		uv.set(0.25f, 0.0f);	break;
	case 2:		uv.set(0.50f, 0.0f);	break;
	case 3:		uv.set(0.75f, 0.0f);	break;
	default:	uv.set(0.00f, 0.0f);	break;
	}
}


void CUICellContainer::SetCellsCapacity(const Ivector2& c)
{
	m_cellsCapacity				= c;
	m_cells.resize				(c.x*c.y);
	ReinitSize					();
}

void CUICellContainer::SetCellSize(const float& size, const EScaling& scaling)
{
	m_cellSize					= size;
	m_cellScaling				= scaling;
	ReinitSize					();
}

void CUICellContainer::SetCellsSpacing(const float& size, const EScaling& scaling)
{
	m_spacingSize				= size;
	m_spacingScaling			= scaling;
	ReinitSize					();
}

Ivector2 CUICellContainer::TopVisibleCell()
{
	return Ivector2().set	(0, iFloor(m_pParentDragDropList->ScrollPos()/float(CellSize().y+CellsSpacing().y)));
}

CUICell& CUICellContainer::GetCellAt(const Ivector2& pos)
{
	R_ASSERT			(ValidCell(pos));
	CUICell&	c		= m_cells[m_cellsCapacity.x*pos.y+pos.x];
	return				c;
}

Ivector2 CUICellContainer::GetItemPos(CUICellItem* itm)
{
	for(int x=0; x<m_cellsCapacity.x ;++x)
		for(int y=0; y<m_cellsCapacity.y ;++y){
			Ivector2 p;
			p.set(x,y);
		if(GetCellAt(p).m_item==itm)
			return p;
		}

		R_ASSERT(0);
		return Ivector2().set(-1,-1);
}

u32 CUICellContainer::GetCellsInRange(const Irect& rect, UI_CELLS_VEC& res)
{
	res.clear_not_free				();
	for(int x=rect.x1;x<=rect.x2;++x)
		for(int y=rect.y1;y<=rect.y2;++y)
			res.push_back	(GetCellAt(Ivector2().set(x,y)));

	res.erase(std::unique(res.begin(), res.end()),res.end());
	return res.size();
}

void CUICellContainer::ReinitSize()
{
	Ivector2							sz;
	sz.add								(CellsSpacing(), CellSize());
	sz.mul								(CellsCapacity());
	sz.sub								(CellsSpacing());

	SetWndSize							(Fvector2().set(sz.x,sz.y));
	m_pParentDragDropList->ReinitScroll	();
}

void CUICellContainer::Grow()
{
	SetCellsCapacity	(Ivector2().set(m_cellsCapacity.x,m_cellsCapacity.y+1));
}

void CUICellContainer::Shrink()
{
}

bool CUICellContainer::ValidCell(const Ivector2& pos) const
{
	return !(pos.x<0 || pos.y<0 || pos.x>=m_cellsCapacity.x || pos.y>=m_cellsCapacity.y);
}

void CUICellContainer::ClearAll(bool bDestroy)
{
	for (auto& cell : m_cells)
		cell.Clear						();

	while (!m_ChildWndList.empty())
	{
		CUIWindow* w					= m_ChildWndList.back();
		CUICellItem* wc					= smart_cast<CUICellItem*>(w);
		VERIFY							(!wc->IsAutoDelete());
		DetachChild						(wc);	
		
		while (wc->ChildsCount())
		{
			CUICellItem* ci				= wc->PopChild(nullptr);
			R_ASSERT					(ci->ChildsCount() == 0);
			if (bDestroy)
				ci->destroy				();
		}
		
		if (bDestroy)
			wc->destroy					();
	}
}

Ivector2 CUICellContainer::PickCell(const Fvector2& abs_pos)
{
	Ivector2							res;
	Fvector2							ap;
	GetAbsolutePos						(ap);
	ap.sub								(abs_pos);
	ap.mul								(-1.f);
	res.x								= iFloor(ap.x/(CellSize().x+CellsSpacing().x*(m_cellsCapacity.x-1)/m_cellsCapacity.x));
	res.y								= iFloor(ap.y/(CellSize().y+CellsSpacing().y*(m_cellsCapacity.y-1)/m_cellsCapacity.y));
	if (!ValidCell(res))
		res.set							(-1.f, -1.f);
	return								res;
}

void CUICellContainer::Draw()
{
	Frect								clientArea;
	m_pParentDragDropList->GetClientArea(clientArea);

	Ivector2 cell_cnt					= m_pParentDragDropList->CellsCapacity();
	if (cell_cnt.x == 0 || cell_cnt.y == 0)
		return;
	
	const Ivector2 cell_size			= CellSize();
	const Ivector2 cell_spacing			= CellsSpacing();
	Ivector2 cell_sz					= cell_size;
	cell_sz.add							(cell_spacing);

	Irect								tgt_cells;
	tgt_cells.lt						= TopVisibleCell();
	Fvector2 cell_szf					= { static_cast<float>(cell_sz.x), static_cast<float>(cell_sz.y) };
	tgt_cells.x2						= iFloor((clientArea.width() + cell_szf.x - EPS) / cell_szf.x) + tgt_cells.lt.x;
	tgt_cells.y2						= iFloor((clientArea.height() + cell_szf.y - EPS) / cell_szf.y) + tgt_cells.lt.y;

	clamp								(tgt_cells.x2, 0, cell_cnt.x - 1);
	clamp								(tgt_cells.y2, 0, cell_cnt.y - 1);

	GetCellsInRange						(tgt_cells, m_cells_to_draw);

	// fill cell buffer
	u32 max_prim_cnt					= ((tgt_cells.width() + 1) * (tgt_cells.height() + 1) * 6);
	UIRender->StartPrimitive			(max_prim_cnt, IUIRender::ptTriList, UI().m_currentPointType);

	if (grid_render)
		render_grid						(tgt_cells, cell_size, cell_spacing, grid_render_empty);

	UI().PushScissor					(clientArea);

	UIRender->SetShader					(*hShader);
	UIRender->FlushPrimitive			();

	//draw shown items in range
	for (auto& cell : m_cells_to_draw) // all cells
		if (cell.m_item && (cell.m_item->m_drawn_frame != Device.dwFrame))
			cell.m_item->Draw			();

	UI().PopScissor						();
}

void CUICellContainer::render_grid(Irect CR$ tgt_cells, Ivector2 CR$ cell_size, Ivector2 CR$ cell_spacing, bool with_empty)
{
	static const Fvector2 pts[6] =		{
		{0.0f, 0.0f},	{1.0f, 0.0f},	{1.0f, 1.0f},
		{0.0f, 0.0f},	{1.0f, 1.0f},	{0.0f, 1.0f}
	};
	static const float ty				= 1.f;
	static const float tx				= .25f;
	static const Fvector2 uvs[6]		= {
		{0.0f, 0.0f},	{tx, 0.0f},		{tx, ty},
		{0.0f, 0.0f},	{tx, ty},		{0.0f, ty}
	};

	Fvector2							lt_abs_pos;
	GetAbsolutePos						(lt_abs_pos);

	Fvector2 drawLT						= {
		lt_abs_pos.x + static_cast<float>(tgt_cells.lt.x * (cell_size.x + 2 * cell_spacing.x)),
		lt_abs_pos.y + static_cast<float>(tgt_cells.lt.y * (cell_size.y + 2 * cell_spacing.y))
	};
	UI().ClientToScreenScaled			(drawLT, drawLT.x, drawLT.y);

	// calculate cell size in screen pixels
	Fvector2							f_len, sp_len;
	UI().ClientToScreenScaled(f_len, static_cast<float>(cell_size.x), static_cast<float>(cell_size.y) );
	UI().ClientToScreenScaled(sp_len, static_cast<float>(cell_spacing.x), static_cast<float>(cell_spacing.y) );

	for (int x = 0; x <= tgt_cells.width(); ++x)
	{
		for (int y = 0; y <= tgt_cells.height(); ++y)
		{
			Ivector2 cpos				= { x, y };
			cpos.add					(TopVisibleCell());
			CUICell& ui_cell			= GetCellAt(cpos);
			if (!with_empty && ui_cell.Empty())
				continue;

			u8 select_mode				= 0;
			if (!ui_cell.Empty())
			{
				if (ui_cell.m_item->m_cur_mark)
					select_mode			= 2;
				else if (ui_cell.m_item->m_selected)
					select_mode			= 1;
				else if (ui_cell.m_item->m_select_armament)
					select_mode			= 3;
				else if (ui_cell.m_item->m_select_equipped)
					select_mode			= 2;
			}

			Fvector2					tp;
			GetTexUVLT					(tp, tgt_cells.x1 + x, tgt_cells.y1 + y, select_mode);

			Fvector2 rect_offset		= {
				drawLT.x + f_len.x * x + sp_len.x * x,
				drawLT.y + f_len.y * y + sp_len.y * y
			};

			for (u32 k = 0; k < 6; ++k)
			{
				const Fvector2& p		= pts[k];
				const Fvector2& uv		= uvs[k];
				UIRender->PushPoint		(
					iFloor(rect_offset.x + p.x * (f_len.x)) - 0.5f,
					iFloor(rect_offset.y + p.y * (f_len.y)) - 0.5f,
					0,
					m_pParentDragDropList->back_color,
					tp.x + uv.x, tp.y + uv.y
				);
			}
		}
	}
}

void CUICellContainer::clear_select_armament()
{
	for (auto& cell : m_cells)
		if (cell.m_item)
			cell.m_item->m_select_armament = false;
}
