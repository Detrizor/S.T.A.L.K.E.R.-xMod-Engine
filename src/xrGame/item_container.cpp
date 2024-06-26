#include "stdafx.h"
#include "item_container.h"
#include "ui/UIActorMenu.h"
#include "uigamecustom.h"

CInventoryContainer::CInventoryContainer(CGameObject* obj) : CModule(obj)
{
	m_Capacity							= pSettings->r_float(O.cNameSect(), "capacity");
	m_Items.clear						();
	InvalidateState						();

	if (cast<PIItem>())
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

void CInventoryContainer::InvalidateState()
{
	for (int i = eWeight; i <= eCost; i++)
		Sum(i)							= flt_max;
}

float CInventoryContainer::Get(EEventTypes type)
{
	if (Sum(type) == flt_max)
	{
		Sum(type)						= 0.f;
		for (auto I : m_Items)
			Sum(type)					+= I->O.Aboba(type);
	}
	return								Sum(type);
}

void CInventoryContainer::OnInventoryAction C$(PIItem item, bool take)
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

float CInventoryContainer::aboba o$(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eOnChild:
		{
			PIItem item					= cast<PIItem>((CObject*)data);
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

void CInventoryContainer::ParentCheck C$(PIItem item, bool add)
{
	if (O.H_Parent())
	{
		CInventoryOwner* io				= O.H_Parent()->Cast<CInventoryOwner*>();
		if (io)
			io->inventory().CheckArtefact(item, add);
		else
			O.H_Parent()->Cast<CInventoryContainer*>()->ParentCheck(item, add);
	}
}

bool CInventoryContainer::CanTakeItem(PIItem item)
{
	if (item->cast<CInventoryContainer*>() == this)
		return							false;
	if (fEqual(m_Capacity, 0.f))
		return							true;
	float vol							= Get(eVolume);
	return								(fLessOrEqual(vol, m_Capacity) && fLessOrEqual(vol + item->Volume(), m_Capacity + 0.1f));
}

void CInventoryContainer::AddAvailableItems(TIItemContainer& items_container) const
{
	for (auto I : m_Items)
		items_container.push_back		(I);
}

bool CInventoryContainer::ArtefactIsolation(bool own) const
{
	if (m_ArtefactIsolation)
		return							true;

	if (!own && O.H_Parent())
	{
		CInventoryContainer* cont		= O.H_Parent()->Cast<CInventoryContainer*>();
		if (cont)
			return						cont->ArtefactIsolation();
	}

	return								false;
}

float CInventoryContainer::RadiationProtection(bool own) const
{
	float res							= m_RadiationProtection;

	if (!own && O.H_Parent())
	{
		CInventoryContainer* cont		= O.H_Parent()->Cast<CInventoryContainer*>();
		if (cont)
			res							*= cont->RadiationProtection();
	}

	return								res;
}
