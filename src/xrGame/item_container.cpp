#include "stdafx.h"
#include "IItemContainer.h"
#include "Level.h"

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

void CContainerObject::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent					(P, type);
	CInventoryContainer::OnEvent		(P, type);
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