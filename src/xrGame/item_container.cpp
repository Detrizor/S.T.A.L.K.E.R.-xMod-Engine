#include "stdafx.h"
#include "item_container.h"
#include "ui/UIActorMenu.h"
#include "uigamecustom.h"

CInventoryContainer::CInventoryContainer()
{
	m_capacity							= 0.f;
}

DLL_Pure* CInventoryContainer::_construct()
{
	m_object							= smart_cast<CGameObject*>(this);
	return								m_object;
}

void CInventoryContainer::Load(LPCSTR section)
{
	m_capacity							= pSettings->r_float(section, "capacity");
}

void CInventoryContainer::OnInventoryAction(PIItem item, u16 actionType) const
{
	CUIActorMenu* actor_menu				= (CurrentGameUI()) ? &CurrentGameUI()->GetActorMenu() : NULL;
	if (actor_menu && actor_menu->IsShown())
	{
		u8 res_zone							= 0;
		if (smart_cast<CInventoryContainer*>(actor_menu->GetBag()) == this)
			res_zone						= 3;
		else if (!m_object->H_Parent())
			res_zone						= 2;

		if (res_zone > 0)
			actor_menu->OnInventoryAction	(item, actionType, res_zone);
	}
}

void CInventoryContainer::OnEventImpl(u16 type, u16 id, CObject* itm, bool dont_create_shell)
{
	PIItem item							= smart_cast<PIItem>(itm);
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		m_items.push_back				(item);
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		m_items.erase					(std::find(m_items.begin(), m_items.end(), item));
		break;
	}
	OnInventoryAction					(item, type);
}

bool CInventoryContainer::CanTakeItem(PIItem item) const
{
	if (smart_cast<CInventoryContainer*>(item) == this)
		return							false;
	if (fEqual(m_capacity, 0.f))
		return							true;
	float vol							= ItemsVolume();
	return								(fLessOrEqual(vol, m_capacity) && fLessOrEqual(vol + item->Volume(), m_capacity + 0.1f));
}

void CInventoryContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_items)
		items_container.push_back		(I);
}

float CInventoryContainer::ItemsWeight() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Weight();
	return								res;
}

float CInventoryContainer::ItemsVolume() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Volume();
	return								res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CContainerObject::CContainerObject()
{
	m_content_volume_scale				= true;
	m_stock								= "";
	m_stock_count						= 0;
	m_modules.push_back					(xr_new<CItemStorage>(this));
}

DLL_Pure* CContainerObject::_construct()
{
	CInventoryContainer::_construct		();
	inherited::_construct				();
	return								this;
}

void CContainerObject::Load(LPCSTR section)
{
	inherited::Load						(section);
	CInventoryContainer::Load			(section);
	m_content_volume_scale				= !!pSettings->r_bool(section, "content_volume_scale");
	m_stock								= pSettings->r_string(section, "stock");
	m_stock_count						= pSettings->r_u32(section, "stock_count");
}

float CContainerObject::Weight() const
{
	float res							= inherited::Weight();
	res									+= ItemsWeight();
	return								res;
}

float CContainerObject::Volume() const
{
	float res							= inherited::Volume();
	if (m_content_volume_scale)
		res								+= ItemsVolume();
	return								res;
}

u32 CContainerObject::Cost() const
{
	u32 res								= inherited::Cost();
	if (!Empty())
		for (TIItemContainer::const_iterator I = Items().begin(), E = Items().end(); I != E; I++)
			res						+= (*I)->Cost();
	return								res;
}
