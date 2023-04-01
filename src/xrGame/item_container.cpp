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

void CInventoryContainer::OnInventoryAction C$(PIItem item, bool take)
{
	CUIActorMenu* actor_menu			= (CurrentGameUI()) ? &CurrentGameUI()->GetActorMenu() : NULL;
	if (actor_menu && actor_menu->IsShown())
	{
		u8 res_zone						= 0;
		if (smart_cast<CInventoryContainer*>(actor_menu->GetBag()) == this)
			res_zone					= 3;
		else if (!m_object->H_Parent())
			res_zone					= 2;

		if (res_zone > 0)
			actor_menu->OnInventoryAction(item, take, res_zone);
	}
}

bool CInventoryContainer::CanTakeItem(PIItem item) const
{
	if (smart_cast<CInventoryContainer*>(item) == this)
		return							false;
	if (fEqual(m_capacity, 0.f))
		return							true;
	float vol							= _Volume();
	return								(fLessOrEqual(vol, m_capacity) && fLessOrEqual(vol + item->Volume(), m_capacity + 0.1f));
}

void CInventoryContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_items)
		items_container.push_back		(I);
}

void CInventoryContainer::_OnChild(CObject* obj, bool take)
{
	PIItem item							= smart_cast<PIItem>(obj);
	if (item)
	{
		if (take)
			m_items.push_back			(item);
		else
			m_items.erase				(std::find(m_items.begin(), m_items.end(), item));
		OnInventoryAction				(item, take);
	}
}

float CInventoryContainer::_Weight() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Weight();
	return								res;
}

float CInventoryContainer::_Volume() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Volume();
	return								res;
}

float CInventoryContainer::_Cost() const
{
	float res							= 0.f;
	for (auto I : m_items)
		res								+= I->Cost();
	return								res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CContainerObject::CContainerObject()
{
	m_content_volume_scale				= true;
	m_stock								= "";
	m_stock_count						= 0;
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
