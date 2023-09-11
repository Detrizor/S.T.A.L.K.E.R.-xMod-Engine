#include "stdafx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "../Weapon.h"
#include "UIDragDropListEx.h"
#include "UIProgressBar.h"

#include "addon_owner.h"
#include "addon.h"

namespace detail 
{

struct is_helper_pred
{
	bool operator ()(CUICellItem* child)
	{
		return child->IsHelper();
	}

}; // struct is_helper_pred

} //namespace detail 

CUIInventoryCellItem::CUIInventoryCellItem(CInventoryItem* itm)
{
	if (!itm)
		return;

	m_section._set									(itm->m_section_id);
	m_pData											= (void*)itm;

	inherited::SetShader							(InventoryUtilities::GetEquipmentIconsShader(m_section));
	inherited::SetTextureRect						(itm->GetIconRect());
	inherited::SetStretchTexture					(true);
	m_grid_size										= InventoryUtilities::CalculateIconSize(itm->GetIconRect(), pSettings->r_float(itm->m_section_id, "icon_scale"), m_TextureMargin);

	//Alundaio; Layered icon
	u8 itrNum = 1;
	LPCSTR field = "1icon_layer";
	while (pSettings->line_exist(itm->m_section_id, field))
	{
		string32 buf;

		LPCSTR section = pSettings->r_string(itm->m_section_id, field);
		if (!section)
			continue;

		Fvector2 offset;
		offset.x = pSettings->r_float(itm->m_section_id, strconcat(sizeof(buf),buf,std::to_string(itrNum).c_str(),"icon_layer_x"));
		offset.y = pSettings->r_float(itm->m_section_id, strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_y"));

		LPCSTR field_scale = strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_scale");
		float scale = pSettings->line_exist(itm->m_section_id, field_scale) ? pSettings->r_float(itm->m_section_id, field_scale) : 1.0f;

		//LPCSTR field_color = strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_color");
		//u32 color = pSettings->line_exist(itm->m_section_id, field_color) ? pSettings->r_color(itm->m_section_id, field_color) : 0;

		CreateLayer(section, offset, scale);

		itrNum++;

		field = strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer");
	}
	//-Alundaio
}

CUIInventoryCellItem::CUIInventoryCellItem(shared_str section)
{
	m_section._set						(section);
	m_pData								= NULL;

	Frect								tex_rect;
	CInventoryItem::ReadIcon			(tex_rect, *section);
	
	inherited::SetShader				(InventoryUtilities::GetEquipmentIconsShader(section));
	inherited::SetTextureRect			(tex_rect);
	inherited::SetStretchTexture		(true);
	m_grid_size							= InventoryUtilities::CalculateIconSize(tex_rect, pSettings->r_float(section, "icon_scale"), m_TextureMargin);

	//Alundaio; Layered icon
	u8 itrNum = 1;
	LPCSTR field = "1icon_layer";
	while (pSettings->line_exist(section, field))
	{
		string32 buf;

		LPCSTR sect = pSettings->r_string(section, field);
		if (!sect)
			continue;

		Fvector2 offset;
		offset.x = pSettings->r_float(section, strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_x"));
		offset.y = pSettings->r_float(section, strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_y"));

		LPCSTR field_scale = strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer_scale");
		float scale = pSettings->line_exist(section, field_scale) ? pSettings->r_float(section, field_scale) : 1.0f;

		CreateLayer(sect, offset, scale);

		itrNum++;

		field = strconcat(sizeof(buf), buf, std::to_string(itrNum).c_str(), "icon_layer");
	}
	//-Alundaio

	m_class_id = TEXT2CLSID(pSettings->r_string(section, "class"));
}

void CUIInventoryCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{

	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		(*it)->m_icon = InitLayer((*it)->m_icon, (*it)->m_name, (*it)->offset, parent_list->GetVerticalPlacement(), (*it)->m_scale);
	}
}


bool CUIInventoryCellItem::EqualTo(CUICellItem* itm)
{
	CUIInventoryCellItem* ci = smart_cast<CUIInventoryCellItem*>(itm);
	if (!itm)
		return			false;
	if (m_section != ci->m_section)
		return			false;
	PIItem item			= object();
	PIItem ci_item		= ci->object();
	if (item && ci_item)
	{
		if (!fsimilar(item->GetCondition(), ci_item->GetCondition(), 0.01f))
			return		false;
		if (!item->equal_upgrades(ci_item->upgardes()))
			return		false;
	}
	return				true;
}

bool CUIInventoryCellItem::IsHelperOrHasHelperChild()
{
	return std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) > 0 || IsHelper();
}

CUIDragItem* CUIInventoryCellItem::CreateDragItem()
{
	if (IsHelperOrHasHelperChild())
		return NULL;

	CUIDragItem* i = inherited::CreateDragItem();
	CUIStatic* s = NULL;

	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		s = xr_new<CUIStatic>(); 
		s->SetAutoDelete(true);
		s->SetShader(InventoryUtilities::GetEquipmentIconsShader(m_section));
		InitLayer(s, (*it)->m_name, (*it)->offset, false, (*it)->m_scale);
		s->SetTextureColor(i->wnd()->GetTextureColor());
		i->wnd()->AttachChild(s);
	}

	return				i;
}

void CUIInventoryCellItem::SetTextureColor(u32 color)
{	
	inherited::SetTextureColor(color);
	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		if ((*it)->m_icon)
			(*it)->m_icon->SetTextureColor(color);
	}
}

bool CUIInventoryCellItem::IsHelper ()
{
	PIItem item		= object();
	return			(item) ? item->is_helper_item() : false;
}

void CUIInventoryCellItem::SetIsHelper (bool is_helper)
{
	object()->set_is_helper(is_helper);
}

//Alundaio
void CUIInventoryCellItem::RemoveLayer(SIconLayer* layer)
{
	if (m_layers.empty())
		return;

	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		if ((*it) == layer)
		{
			DetachChild((*it)->m_icon);
			m_layers.erase(it);
			return;
		}
	}
}

void CUIInventoryCellItem::CreateLayer(LPCSTR section, Fvector2 offset, float scale)
{
	SIconLayer* layer = xr_new<SIconLayer>();
	layer->m_name = section;
	layer->offset = offset;
	//layer->m_color = color;
	layer->m_scale = scale;
	m_layers.push_back(layer);
}

CUIStatic* CUIInventoryCellItem::InitLayer(CUIStatic* s, LPCSTR section, Fvector2 addon_offset, bool b_rotate, float scale)
{
	//--xd fix this like addons
	if (!s)
	{
		s = xr_new<CUIStatic>();
		s->SetAutoDelete(true);
		AttachChild(s);
		s->SetShader(InventoryUtilities::GetEquipmentIconsShader(section));
		s->SetTextureColor(GetTextureColor());
	}

	Frect					tex_rect;
	Fvector2				base_scale;

	if (Heading())
	{
		base_scale.x		= (GetHeight() / GetTextureRect().width()) * scale;
		base_scale.y		= (GetWidth() / GetTextureRect().height()) * scale;
	}
	else
	{
		base_scale.x		= (GetWidth() / GetTextureRect().width()) * scale;
		base_scale.y		= (GetHeight() / GetTextureRect().height()) * scale;
	}

	CInventoryItem::ReadIcon(tex_rect, section);
	Fvector2 cell_size;
	tex_rect.getsize(cell_size);
	cell_size.mul(base_scale);

	if (b_rotate)
	{
		s->SetWndSize(Fvector2().set(cell_size.y, cell_size.x));
		Fvector2 new_offset;
		new_offset.x = GetWidth() - addon_offset.y * base_scale.y - cell_size.y;
		new_offset.y = addon_offset.x * base_scale.y;
		addon_offset = new_offset;
	}
	else
	{
		s->SetWndSize(cell_size);
		addon_offset.mul(base_scale);
	}

	s->SetWndPos(addon_offset);
	s->SetTextureRect(tex_rect);
	s->SetStretchTexture(true);

	s->EnableHeading(b_rotate);

	if (b_rotate)
	{
		s->SetHeading(GetHeading());
		s->SetHeadingPivot(Fvector2().set(0.0f, 0.0f), Fvector2().set(s->GetWndSize().x, 0.f), true);
	}

	return s;
}

//-Alundaio
void CUIInventoryCellItem::Update()
{
	bool b = Heading();
	inherited::Update();

	UpdateConditionProgressBar(); //Alundaio
	UpdateItemText();

	u32 color = GetTextureColor();
	if ( IsHelper() && !ChildsCount() )
	{
		color = 0xbbbbbbbb;
	}
	else if ( IsHelperOrHasHelperChild() )
	{
		color = 0xffffffff;
	}

	SetTextureColor(color);

	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		(*it)->m_icon = InitLayer((*it)->m_icon, (*it)->m_name, (*it)->offset, Heading(), (*it)->m_scale);
		(*it)->m_icon->SetTextureColor(color);
	}
}

void CUIInventoryCellItem::UpdateItemText()
{
	const u32	helper_count	=  	(u32)std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) 
									+ (u32)IsHelper();

	const u32	count			=	ChildsCount() + 1 - helper_count;
	string32	str;

	if ( count > 1 || helper_count )
	{
		xr_sprintf						( str, "x%d", count );
		m_text->TextItemControl()->SetText	( str );
		m_text->Show					( true );
	}
	else
	{
		xr_sprintf						( str, "");
		m_text->TextItemControl()->SetText	( str );
		m_text->Show					( false );
	}
	m_text->AdjustWidthToText			();
	m_text->AdjustHeightToText			();
}

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm)
:inherited(itm)
{}

bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
	if(!inherited::EqualTo(itm))	return false;

	CUIAmmoCellItem* ci				= smart_cast<CUIAmmoCellItem*>(itm);
	if(!ci)							return false;

	return					( (object()->cNameSect() == ci->object()->cNameSect()) );
}

CUIDragItem* CUIAmmoCellItem::CreateDragItem()
{
	return IsHelper() ? NULL : inherited::CreateDragItem();
}

u32 CUIAmmoCellItem::CalculateAmmoCount()
{
	u32 total = IsHelper() ? 0 : object()->GetAmmoCount();
	for (const auto child : m_childs)
	{
		if (!child->IsHelper())
			total += ((CUIAmmoCellItem*)child)->object()->GetAmmoCount();
	}

	return total;
}

void CUIAmmoCellItem::UpdateItemText()
{
	m_text->Show								(false);
	if (!m_custom_draw)
	{
		const u32 total							= CalculateAmmoCount();
		if (1 < total)
		{
			string32							str;
			xr_sprintf							(str, "%d", total);
			m_text->TextItemControl()->SetText	(str);
			m_text->AdjustWidthToText			();
			m_text->AdjustHeightToText			();
			m_text->Show						(true);
		}
	}
}

CUIAddonOwnerCellItem::SUIAddonSlot::SUIAddonSlot(SAddonSlot CR$ slot)
{
	name								= slot.name;
	type								= slot.type;
	addon_name							= 0;
	addon_type							= 0;
	addon_index							= 0;
	addon_icon							= NULL;
	icon_offset							= { 0.f, 0.f };
	forwarded							= !!slot.forwarded_slot;
}

CUIAddonOwnerCellItem::CUIAddonOwnerCellItem(CAddonOwner* itm) : inherited(itm->cast<PIItem>())
{
	m_slots.clear						();

	for (auto S : itm->AddonSlots())
	{
		SUIAddonSlot* s					= xr_new<SUIAddonSlot>(*S);
		m_slots.push_back				(s);

		if (S->addon)
		{
			s->addon_name				= S->addon->Section();
			s->addon_type				= S->addon->GetInvIconType();
			s->addon_index				= S->addon->GetInvIconIndex();
			s->icon_offset				= S->icon_offset;
			s->icon_offset.sub			(S->addon->IconOffset());
			s->addon_icon				= xr_new<CUIStatic>();
			s->addon_icon->SetAutoDelete(true);
			AttachChild					(s->addon_icon);
			s->addon_icon->SetTextureColor(GetTextureColor());
			if (S->magazine)
				s->addon_icon->SetBackgroundDraw(true);
		}
	}

	float icon_scale					= pSettings->r_float(itm->O.cNameSect(), "icon_scale");
	Frect res_rect						= GetTextureRect();
	res_rect.mul						(icon_scale, icon_scale);
	Frect tex_rect						= GetTextureRect();
	tex_rect.mul						(icon_scale, icon_scale);

	for (auto s : m_slots)
	{
		if (s->addon_icon)
		{
			res_rect.left				= min(res_rect.left, tex_rect.left + s->icon_offset.x);
			res_rect.top				= min(res_rect.top, tex_rect.top + s->icon_offset.y);

			Frect						addon_rect;
			CInventoryItem::ReadIcon	(addon_rect, *s->addon_name);
			float icon_scale			= pSettings->r_float(s->addon_name, "icon_scale");
			res_rect.right				= max(res_rect.right, tex_rect.left + s->icon_offset.x + icon_scale * addon_rect.width());
			res_rect.bottom				= max(res_rect.bottom, tex_rect.top + s->icon_offset.y + icon_scale * addon_rect.height());
		}
	}

	m_grid_size							= InventoryUtilities::CalculateIconSize(tex_rect, m_TextureMargin, res_rect);
}

CUIAddonOwnerCellItem::CUIAddonOwnerCellItem(shared_str section) : inherited(section)
{
	m_slots.clear						();

	VSlots								slots;
	CAddonOwner::LoadAddonSlots			(pSettings->r_string(section, "slots"), slots);
	for (auto slot : slots)
	{
		SUIAddonSlot* s					= xr_new<SUIAddonSlot>(*slot);
		m_slots.push_back(s);
	}

	slots.clear							();
}

CUIAddonOwnerCellItem::~CUIAddonOwnerCellItem()
{
	m_slots.clear						();
}

void CUIAddonOwnerCellItem::Draw()
{	
	inherited::Draw						();
	if (m_upgrade && m_upgrade->IsShown())
		m_upgrade->Draw					();
};

void CUIAddonOwnerCellItem::Update()
{
	bool b								= Heading();
	inherited::Update					();
	if (b != Heading())
	{
		for (auto s : m_slots)
		{
			if (s->addon_icon)
				InitAddon				(s->addon_icon, *s->addon_name, s->addon_type, s->addon_index, s->icon_offset, Heading());
		}
	}
}

void CUIAddonOwnerCellItem::SetTextureColor(u32 color)
{
	inherited::SetTextureColor			(color);
	for (auto s : m_slots)
	{
		if (s->addon_icon)
			s->addon_icon->SetTextureColor(color);
	}
}

void CUIAddonOwnerCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{
	for (auto s : m_slots)
	{
		if (s->addon_icon)
			InitAddon					(s->addon_icon, *s->addon_name, s->addon_type, s->addon_index, s->icon_offset, parent_list->GetVerticalPlacement());
	}
}

void CUIAddonOwnerCellItem::InitAddon(CUIStatic* s, LPCSTR section, u8 type, u8 index, Fvector2 addon_offset, bool b_rotate, bool drag)
{
	s->SetShader						(InventoryUtilities::GetEquipmentIconsShader(section));

	Frect								tex_rect;
	Fvector2							base_scale;

	if (Heading())
	{
		base_scale.x					= (1.f - m_TextureMargin.left - m_TextureMargin.right) * GetHeight() / GetTextureRect().width();
		base_scale.y					= (1.f - m_TextureMargin.top - m_TextureMargin.bottom) * GetWidth() / GetTextureRect().height();
	}
	else
	{
		base_scale.x					= (1.f - m_TextureMargin.left - m_TextureMargin.right) * GetWidth() / GetTextureRect().width();
		base_scale.y					= (1.f - m_TextureMargin.top - m_TextureMargin.bottom) * GetHeight() / GetTextureRect().height();
	}
	base_scale.div						(pSettings->r_float(m_section, "icon_scale"));

	CInventoryItem::ReadIcon			(tex_rect, section, type, index);
	Fvector2							cell_size;
	tex_rect.getsize					(cell_size);
	cell_size.mul						(base_scale);
	cell_size.mul						(pSettings->r_float(section, "icon_scale"));

	if (b_rotate)
	{
		s->SetWndSize					(Fvector2().set(cell_size.y, cell_size.x) );
		Fvector2						new_offset;
		new_offset.x					= GetWidth() - addon_offset.y * base_scale.y - cell_size.y;
		new_offset.y					= addon_offset.x * base_scale.x;
		addon_offset					= new_offset;
		
		if (!drag)
		{
			addon_offset.x				-= GetWidth() * m_TextureMargin.top;
			addon_offset.y				+= GetHeight() * m_TextureMargin.left;
		}
	}
	else
	{
		s->SetWndSize					(cell_size);
		addon_offset.mul				(base_scale);

		if (!drag)
			addon_offset.add			(GetWndSize().mul(m_TextureMargin.lt));
	}

	s->SetWndPos						(addon_offset);
	s->SetTextureRect					(tex_rect);
	s->SetStretchTexture				(true);

	s->EnableHeading					(b_rotate);
		
	if (b_rotate)
	{
		s->SetHeading					(GetHeading());
		s->SetHeadingPivot				(Fvector2().set(0.0f,0.0f), Fvector2().set(s->GetWndSize().x, 0.f), true);
	}
}

CUIDragItem* CUIAddonOwnerCellItem::CreateDragItem()
{
	CUIDragItem* i						= inherited::CreateDragItem();
	CUIStatic* st						= NULL;

	for (auto s : m_slots)
	{
		if (s->addon_icon)
		{
			st							= xr_new<CUIStatic>();
			st->SetAutoDelete			(true);
			InitAddon					(st, *s->addon_name, s->addon_type, s->addon_index, s->icon_offset, false, true);
			st->SetTextureColor			(i->wnd()->GetTextureColor());
			i->wnd()->AttachChild		(st);
		}
	}

	return								i;
}

CBuyItemCustomDrawCell::CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont)
{
	m_pFont		= pFont;
	VERIFY		(xr_strlen(str)<16);
	xr_strcpy		(m_string,str);
}

void CBuyItemCustomDrawCell::OnDraw(CUICellItem* cell)
{
	Fvector2							pos;
	cell->GetAbsolutePos				(pos);
	UI().ClientToScreenScaled			(pos, pos.x, pos.y);
	m_pFont->Out						(pos.x, pos.y, m_string);
	m_pFont->OnRender					();
}
