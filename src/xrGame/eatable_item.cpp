#include "stdafx.h"
#include "eatable_item.h"
#include "UIGameCustom.h"

void CEatableItem::OnH_A_Independent() 
{
	inherited::OnH_A_Independent();
	if(!Useful() && Local() && OnServer())
		DestroyObject();

	if (!Useful())
	{
		setVisible(false);
		setEnabled(false);
	}
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
	if(!Useful()) 
	{
		setVisible(FALSE);
		setEnabled(FALSE);
		m_ready_to_destroy	= true;
	}
	inherited::OnH_B_Independent(just_before_destroy);
}

bool CEatableItem::UseBy(CEntityAlive* entity_alive)
{
	MAmountable* ai				= getModule<MAmountable>();
	float deprate				= (ai && READ_IF_EXISTS(pSettings, r_BOOL, m_section_id, "use1_continuous", FALSE)) ? ai->getDepletionRate() : 1.f;

	SMedicineInfluenceValues	V;
	V.Load						(cNameSect(), deprate);

	CInventoryOwner* IO			= smart_cast<CInventoryOwner*>(entity_alive);
	R_ASSERT					(IO);
	R_ASSERT					(m_pInventory == IO->m_inventory.get());
	R_ASSERT					(H_Parent()->ID() == entity_alive->ID());

	entity_alive->conditions().ApplyInfluence(V, cNameSect());

	for (u8 i = 0; i < (u8)eBoostMaxCount; i++)
	{
		if (pSettings->line_exist(cNameSect().c_str(), ef_boosters_section_names[i]))
		{
			SBooster B;
			B.Load(cNameSect(), (EBoostParams)i);
			entity_alive->conditions().ApplyBooster(B, cNameSect());
		}
	}

	return true;
}
