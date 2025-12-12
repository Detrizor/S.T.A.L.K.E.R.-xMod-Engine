#include "stdafx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "../Weapon.h"
#include "UIDragDropListEx.h"
#include "UIProgressBar.h"

#include "addon_owner.h"
#include "addon.h"

CUIInventoryCellItem::CUIInventoryCellItem(CInventoryItem* item) : CUIInventoryCellItem(item->m_section_id, &item->GetIconRect())
{
	m_pData								= static_cast<void*>(item);
}

CUIInventoryCellItem::CUIInventoryCellItem(shared_str section, Frect* rect)
{
	m_section._set						(section);

	Frect								tex_rect;
	if (rect)
		tex_rect						= *rect;
	else
	{
		CInventoryItem::readIcon		(tex_rect, section.c_str());
		inherited::SetTextureRect		(tex_rect);
	}

	inherited::SetShader				(InventoryUtilities::GetEquipmentIconsShader(section));
	inherited::SetTextureRect			(tex_rect);
	inherited::SetStretchTexture		(true);
	
	m_scale								= pSettings->r_float(section, "icon_scale");
	m_grid_size							= InventoryUtilities::CalculateIconSize(tex_rect, m_scale, m_TextureMargin);

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

CUIDragItem* CUIInventoryCellItem::CreateDragItem()
{
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

	CInventoryItem::readIcon(tex_rect, section);
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

	auto color{ GetTextureColor() };
	SetTextureColor(color);

	for (xr_vector<SIconLayer*>::iterator it = m_layers.begin(); m_layers.end() != it; ++it)
	{
		(*it)->m_icon = InitLayer((*it)->m_icon, (*it)->m_name, (*it)->offset, Heading(), (*it)->m_scale);
		(*it)->m_icon->SetTextureColor(color);
	}
}

void CUIInventoryCellItem::UpdateItemText()
{
	const u32	count			=	ChildsCount() + 1;
	string32	str;

	if ( count > 1)
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

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm) : inherited(itm)
{
}

CUIAmmoCellItem::CUIAmmoCellItem(shared_str const& section) : inherited(section)
{
}

bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
	if (inherited::EqualTo(itm))
		if (auto ci{ smart_cast<CUIAmmoCellItem*>(itm) })
			return (m_section == ci->m_section);

	return false;
}

u32 CUIAmmoCellItem::CalculateAmmoCount(bool recursive)
{
	auto item{ object() };
	auto total{ (item) ? item->GetAmmoCount() : CWeaponAmmo::readBoxSize(m_section.c_str()) };

	if (recursive)
		for (const auto child : m_childs)
			if (auto aci{ dynamic_cast<CUIAmmoCellItem*>(child) })
				total += aci->CalculateAmmoCount(true);

	return total;
}

void CUIAmmoCellItem::UpdateItemText()
{
	m_text->Show								(false);
	if (!m_custom_draw)
	{
		const u32 total							= CalculateAmmoCount(true);
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

CUIAddonOwnerCellItem::SUIAddonSlot::SUIAddonSlot(xptr<CAddonSlot> CR$ slot)
{
	name								= slot->name;
	type								= slot->type;
	icon_offset							= slot->icon_offset;
	icon_step							= slot->icon_step;
	icon_background_draw				= slot->getBackgroundDraw();
	icon_foreground_draw				= slot->getForegroundDraw();
}

void CUIAddonOwnerCellItem::process_slots(VSlots CR$ slots, Fvector2 CR$ forwarded_offset)
{
	for (auto& S : slots)
	{
		if (!S->type)
			continue;

		if (S->addons.size())
		{
			for (auto addon : S->addons)
			{
				auto& s					= m_slots.emplace_back(S);
				s->addon_section		= addon->section();
				
				if (S->getIconDraw())
				{
					Dvector				hpb;
					addon->getLocalTransform().getHPB(hpb);

					if (addon->I && addon->I->areInvIconTypesAllowed())
					{
						if (abs(hpb.z) >= .75f * PI)
							s->addon_type	= 2;
						else if (hpb.z >= PI_DIV_4)
							s->addon_type	= 3;
						else if (hpb.z <= -PI_DIV_4)
							s->addon_type	= 1;
						else
							s->addon_type	= addon->I->getInvIconType();
					}

					s->addon_index		= (addon->I) ? addon->I->GetInvIconIndex() : 0;
					s->icon_offset.add	(forwarded_offset);
					s->icon_offset.sub	(addon->getIconOrigin(s->addon_type));
					s->icon_offset.x	-= s->icon_step * static_cast<float>(addon->getSlotPos());

					s->addon_icon.construct	();
					AttachChild			(s->addon_icon.get());
					s->addon_icon->SetTextureColor(GetTextureColor());
					if (s->icon_background_draw || (hpb.z >= PI_DIV_4 && hpb.z < PI * .75f))
						s->addon_icon->setBackgroundDraw(true);
					else if (s->icon_foreground_draw)
						s->addon_icon->setForegroundDraw(true);
				}

				if (addon->slots)
					process_slots		(*addon->slots, s->icon_offset);
			}
		}
		else
			m_slots.emplace_back		(S);
	}
}

void CUIAddonOwnerCellItem::calculate_grid(shared_str CR$ sect)
{
	Frect res_rect						= GetTextureRect();
	res_rect.mul						(m_scale, m_scale);
	Frect tex_rect						= res_rect;

	for (auto& s : m_slots)
	{
		if (s->addon_icon)
		{
			res_rect.left				= min(res_rect.left, tex_rect.left + s->icon_offset.x);
			res_rect.top				= min(res_rect.top, tex_rect.top + s->icon_offset.y);

			Frect						addon_rect;
			CInventoryItem::readIcon	(addon_rect, *s->addon_section, s->addon_type);
			res_rect.right				= max(res_rect.right, tex_rect.left + s->icon_offset.x + m_scale * addon_rect.width());
			res_rect.bottom				= max(res_rect.bottom, tex_rect.top + s->icon_offset.y + m_scale * addon_rect.height());
		}
	}

	m_grid_size							= InventoryUtilities::CalculateIconSize(tex_rect, m_TextureMargin, res_rect);
}

CUIAddonOwnerCellItem::CUIAddonOwnerCellItem(MAddonOwner* ao) : inherited(ao->O.scast<CInventoryItem*>())
{
	process_slots						(ao->AddonSlots(), vZero2);
	m_base_foreground_draw				= ao->getBaseForegroundDraw();
	calculate_grid						(ao->O.cNameSect());
}

static void process_supplies(shared_str CR$ section, xr_vector<xptr<MAddon>>& supplies, xr_vector<xptr<VSlots>>& sup_slots)
{
	LPCSTR str							= READ_IF_EXISTS(pSettings, r_string, section, "supplies", 0);
	if (str && str[0])
	{
		string128						sect;
		for (int i = 0, e = _GetItemCount(str); i < e; ++i)
		{
			_GetItem					(str, i, sect);
			auto addon					= supplies.emplace_back(nullptr, sect).get();
			if (READ_IF_EXISTS(pSettings, r_bool, sect, "addon_owner", FALSE))
			{
				addon->slots			= sup_slots.emplace_back().get();
				MAddonOwner::loadAddonSlots(sect, *addon->slots);
			}
		}
	}
}

static bool try_insert(MAddon* addon, VSlots& slots)
{
	for (auto& slot : slots)
	{
		if (slot->canTake(addon))
		{
			slot->addons.push_back		(addon);
			if (slot->steps > 1)
				addon->setSlotPos		((addon->isFrontPositioning()) ? slot->steps - addon->getLength(slot->step) : 0);
			return						true;
		}
		else
			for (auto slot_addon : slot->addons)
				if (slot_addon->slots)
					if (try_insert(addon, *slot_addon->slots))
						return			true;
	}
	return								false;
}

CUIAddonOwnerCellItem::CUIAddonOwnerCellItem(shared_str CR$ section) : inherited(section)
{
	VSlots								slots;
	m_base_foreground_draw				= MAddonOwner::loadAddonSlots(section, slots);

	xr_vector<xptr<MAddon>> supplies	= {};
	xr_vector<xptr<VSlots>> sup_slots	= {};
	process_supplies					(section, supplies, sup_slots);
	for (auto& addon : supplies)
		try_insert						(addon.get(), slots);
	for (auto& slot : slots)
		for (auto& addon : slot->addons)
			slot->updateAddonLocalTransform(addon);

	process_slots						(slots, vZero2);
	calculate_grid						(section);
}

void CUIAddonOwnerCellItem::Draw()
{	
	inherited::Draw						();
	if (m_upgrade && m_upgrade->IsShown())
		m_upgrade->Draw					();
}

void CUIAddonOwnerCellItem::Update()
{
	bool b								= Heading();
	inherited::Update					();
	if (b != Heading())
	{
		for (auto& s : m_slots)
		{
			if (s->addon_icon)
				InitAddon				(s->addon_icon.get(), s->addon_section.c_str(), s->addon_type, s->addon_index, s->icon_offset, Heading());
		}
	}
}

void CUIAddonOwnerCellItem::SetTextureColor(u32 color)
{
	inherited::SetTextureColor			(color);
	for (auto& s : m_slots)
	{
		if (s->addon_icon)
			s->addon_icon->SetTextureColor(color);
	}
}

void CUIAddonOwnerCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{
	for (auto& s : m_slots)
	{
		if (s->addon_icon)
			InitAddon					(s->addon_icon.get(), s->addon_section.c_str(), s->addon_type, s->addon_index, s->icon_offset, parent_list->GetVerticalPlacement());
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
	base_scale.div						(m_scale);

	CInventoryItem::readIcon			(tex_rect, section, type, index);
	Fvector2							cell_size;
	tex_rect.getsize					(cell_size);
	cell_size.mul						(base_scale);
	cell_size.mul						(m_scale);

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

	for (auto& s : m_slots)
	{
		if (s->addon_icon)
		{
			st							= xr_new<CUIStatic>();
			st->SetAutoDelete			(true);
			InitAddon					(st, *s->addon_section, s->addon_type, s->addon_index, s->icon_offset, false, true);
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
