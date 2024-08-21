#include "stdafx.h"
#include "item_container.h"
#include "ui/UIActorMenu.h"
#include "uigamecustom.h"

MContainer::MContainer(CGameObject* obj) : CModule(obj)
{
	m_Capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	m_Items.clear						();
	InvalidateState						();

	if (I)
	{
		m_content_volume_scale			= !!pSettings->r_bool(O.cNameSect(), "content_volume_scale");
		m_ArtefactIsolation				= !!pSettings->r_bool(O.cNameSect(), "artefact_isolation");
		m_RadiationProtection			= pSettings->r_float(O.cNameSect(), "radiation_protection");
	}
	else
	{
		m_ArtefactIsolation				= false;
		m_RadiationProtection			= 1.f;
	}
}

void MContainer::InvalidateState()
{
	for (int i = eWeight; i <= eCost; i++)
		Sum(i)							= flt_max;
}

float MContainer::Get(EEventTypes type)
{
	if (Sum(type) == flt_max)
	{
		Sum(type)						= 0.f;
		for (auto I : m_Items)
			Sum(type)					+= I->O.Aboba(type);
	}
	return								Sum(type);
}

void MContainer::OnInventoryAction C$(PIItem item, bool take)
{
	CUIActorMenu* actor_menu			= (CurrentGameUI()) ? &CurrentGameUI()->GetActorMenu() : NULL;
	if (actor_menu && actor_menu->IsShown())
	{
		u8 res_zone						= 0;
		if (actor_menu->GetBag() == this)
			res_zone					= 3;
		else if (!O.H_Parent())
			res_zone					= 2;

		if (res_zone > 0)
			actor_menu->OnInventoryAction(item, take, res_zone);
	}
}

float MContainer::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eOnChild:
		{
			PIItem item					= static_cast<CObject*>(data)->getModule<CInventoryItem>();
			if (!item)
				break;

			if (param)
				m_Items.push_back		(item);
			else
				m_Items.erase			(::std::find(m_Items.begin(), m_Items.end(), item));
			OnInventoryAction			(item, (bool)param);
			InvalidateState				();
			ParentCheck					(item, !!param);

			break;
		}

		case eVolume:
			if (!m_content_volume_scale)
				break;
		case eWeight:
		case eCost:
			return						Get(type);

		case eGetAmount:
			return						Get(eVolume);
		case eGetBar:
		case eGetFill:
		{
			float fill					= Get(eVolume) / m_Capacity;
			if (type == eGetBar)
				return					(fLess(fill, 1.f)) ? fill : -1.f;
			return						fill;
		}
	}

	return								CModule::aboba(type, data, param);
}

void MContainer::ParentCheck C$(PIItem item, bool add)
{
	if (O.H_Parent())
	{
		if (auto io = O.H_Parent()->scast<CInventoryOwner*>())
			io->inventory().CheckArtefact(item, add);
		else
			O.H_Parent()->getModule<MContainer>()->ParentCheck(item, add);
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
		if (auto cont = O.H_Parent()->getModule<MContainer>())
			return						cont->ArtefactIsolation();
	}

	return								false;
}

float MContainer::RadiationProtection(bool own) const
{
	float res							= m_RadiationProtection;

	if (!own && O.H_Parent())
	{
		if (auto cont = O.H_Parent()->getModule<MContainer>())
			res							*= cont->RadiationProtection();
	}

	return								res;
}
