#include "stdafx.h"
#include "item_container.h"
#include "InventoryOwner.h"
#include "Actor.h"
#include "script_game_object.h"

MContainer::MContainer(CGameObject* obj) : CModule(obj)
{
	m_capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	if (I)
	{
		m_content_volume_scale			= pSettings->r_bool(O.cNameSect(), "content_volume_scale");
		m_artefact_isolation			= pSettings->r_bool(O.cNameSect(), "artefact_isolation");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MContainer::sOnChild(CGameObject* obj, bool take)
{
	if (auto item = obj->scast<CInventoryItem*>())
	{
		if (take)
			m_items.push_back			(item);
		else
			m_items.erase_data			(item);

		InvalidateState					();
		ParentCheck						(item, take);
		if (take)
			item->OnMoveToRuck			(SInvItemPlace());

		if (auto ao = O.H_Root()->scast<CInventoryOwner*>())
		{
			auto go						= smart_cast<CGameObject*>(ao);
			auto& callback				= go->callback((take) ? GameObject::eOnItemTake : GameObject::eOnItemDrop);
			callback					(item->O.lua_game_object());
		}
	}
}

float MContainer::sSumItemData(EItemDataTypes type)
{
	switch (type)
	{
	case eVolume:
		if (!m_content_volume_scale)
			return						0.f;
	case eWeight:
	case eCost:
		return							Get(type);
	default:
		FATAL							("wrong item data type");
	}
}

xoptional<float> MContainer::sGetAmount()
{
	return								Get(eVolume);
}

xoptional<float> MContainer::sGetFill()
{
	return								Get(eVolume) / m_capacity;
}

xoptional<float> MContainer::sGetBar()
{
	float fill							= Get(eVolume) / m_capacity;
	return								(fLess(fill, 1.f)) ? fill : -1.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MContainer::InvalidateState()
{
	for (auto& s : m_sum)
		s.drop							();
}

float MContainer::Get(EItemDataTypes type)
{
	if (!m_sum[type])
	{
		m_sum[type]						= 0.f;
		for (auto I : m_items)
			m_sum[type].get()			+= I->getData(type);
	}
	return								m_sum[type].get();
}

void MContainer::ParentCheck C$(PIItem item, bool add)
{
	if (O.H_Parent())
	{
		if (auto io = O.H_Parent()->scast<CInventoryOwner*>())
			io->inventory().checkArtefact(item, add);
		else
			O.H_Parent()->mcast<MContainer>()->ParentCheck(item, add);
	}
}

bool MContainer::CanTakeItem(PIItem item)
{
	if (item->O.getModule<MContainer>() == this)
		return							false;
	if (fEqual(m_capacity, 0.f))
		return							true;
	float vol							= Get(eVolume);
	return								(fLessOrEqual(vol, m_capacity) && fLessOrEqual(vol + item->Volume(), m_capacity + .1f));
}

void MContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_items)
		items_container.push_back		(I);
}

bool MContainer::ArtefactIsolation(bool own) const
{
	if (m_artefact_isolation)
		return							true;

	if (!own && O.H_Parent())
		if (auto cont = O.H_Parent()->mcast<MContainer>())
			return						cont->ArtefactIsolation();

	return								false;
}
