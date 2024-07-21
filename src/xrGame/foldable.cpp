#include "stdafx.h"
#include "foldable.h"
#include "GameObject.h"
#include "xrServer_Objects_ALife.h"
#include "inventory_item_object.h"
#include "addon.h"

shared_str folded_str					= "folded";
shared_str unfolded_str					= "unfolded";

float CFoldable::aboba(EEventTypes type, void* data, int param)
{
	switch (type)
	{
		case eSyncData:
		{
			auto se_obj					= reinterpret_cast<CSE_ALifeDynamicObject*>(data);
			auto m						= se_obj->getModule<CSE_ALifeModuleFoldable>(!!param);
			if (param)
				m->m_status				= static_cast<u8>(m_status);
			else
				setStatus				((m) ? !!m->m_status : m_status);
		}
	}

	return								CModule::aboba(type, data, param);
}

void CFoldable::setStatus(bool status)
{
	m_status							= status;
	O.UpdateBoneVisibility				(folded_str, status);
	O.UpdateBoneVisibility				(unfolded_str, !status);
	I->SetInvIconType					(static_cast<u8>(status));

	if (auto addon = cast<CAddon*>())
		addon->setLowProfile			(status);
}
