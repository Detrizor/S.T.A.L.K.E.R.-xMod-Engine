#include "stdafx.h"
#include "foldable.h"
#include "GameObject.h"
#include "xrServer_Objects_ALife.h"
#include "inventory_item_object.h"
#include "addon.h"
#include "WeaponMagazined.h"

shared_str folded_str					= "folded";
shared_str unfolded_str					= "unfolded";

void MFoldable::sSyncData(CSE_ALifeDynamicObject* se_obj, bool save)
{
	auto m								= se_obj->getModule<CSE_ALifeModuleFoldable>(save);
	if (save)
		m->m_status						= static_cast<u8>(m_status);
	else
		setStatus						((m) ? !!m->m_status : m_status);
}

void MFoldable::on_status_change(bool new_status) const
{
	auto parent							= O.H_Parent();
	while (parent)
	{
		if (auto wpn = parent->scast<CWeaponMagazined*>())
		{
			wpn->onFold					(this, new_status);
			break;
		}
		else
			parent						= parent->H_Parent();
	}
}

void MFoldable::setStatus(bool status)
{
	if (status)
		on_status_change				(status);

	m_status							= status;
	O.UpdateBoneVisibility				(folded_str, status);
	O.UpdateBoneVisibility				(unfolded_str, !status);
	I->SetInvIconType					(static_cast<u8>(status));

	if (!status)
		on_status_change				(status);

	if (auto addon = O.getModule<MAddon>())
		addon->setLowProfile			(status);
}
