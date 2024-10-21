#include "stdafx.h"
#include "item_container.h"
#include "InventoryOwner.h"

MContainer::MContainer(CGameObject* obj) : CModule(obj)
{
	m_Capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	m_Items.clear						();
	InvalidateState						();

	if (I)
	{
		m_content_volume_scale			= !!pSettings->r_BOOL(O.cNameSect(), "content_volume_scale");
		m_ArtefactIsolation				= !!pSettings->r_BOOL(O.cNameSect(), "artefact_isolation");
		m_RadiationProtection			= pSettings->r_float(O.cNameSect(), "radiation_protection");
	}
	else
	{
		m_ArtefactIsolation				= false;
		m_RadiationProtection			= 1.f;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MContainer::sOnChild(CGameObject* obj, bool take)
{
	if (auto item = obj->scast<CInventoryItem*>())
	{
		if (take)
			m_Items.push_back			(item);
		else
			m_Items.erase_data			(item);

		InvalidateState					();
		ParentCheck						(item, take);
		if (take)
			item->OnMoveToRuck			(SInvItemPlace());
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
	return Get(eVolume);
}

xoptional<float> MContainer::sGetFill()
{
	return Get(eVolume) / m_Capacity;
}

xoptional<float> MContainer::sGetBar()
{
	float fill							= Get(eVolume) / m_Capacity;
	return								(fLess(fill, 1.f)) ? fill : -1.f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MContainer::InvalidateState()
{
	for (int i = eWeight; i <= eCost; i++)
		Sum(i)							= flt_max;
}

float MContainer::Get(EItemDataTypes type)
{
	if (Sum(type) == flt_max)
	{
		Sum(type)						= 0.f;
		for (auto I : m_Items)
			Sum(type)					+= I->getData(type);
	}
	return								Sum(type);
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
	if (fEqual(m_Capacity, 0.f))
		return							true;
	float vol							= Get(eVolume);
	return								(fLessOrEqual(vol, m_Capacity) && fLessOrEqual(vol + item->Volume(), m_Capacity + 0.1f));
}

void MContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_Items)
		items_container.push_back		(I);
}

bool MContainer::ArtefactIsolation(bool own) const
{
	if (m_ArtefactIsolation)
		return							true;

	if (!own && O.H_Parent())
	{
		if (auto cont = O.H_Parent()->mcast<MContainer>())
			return						cont->ArtefactIsolation();
	}

	return								false;
}

float MContainer::RadiationProtection(bool own) const
{
	float res							= m_RadiationProtection;

	if (!own && O.H_Parent())
	{
		if (auto cont = O.H_Parent()->mcast<MContainer>())
			res							*= cont->RadiationProtection();
	}

	return								res;
}
