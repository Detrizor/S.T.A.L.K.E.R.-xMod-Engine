#include "stdafx.h"
#include "inventory.h"
#include "weapon.h"
#include "actor.h"
#include "../xrCore/xr_ini.h"
#include "Grenade.h"

static u32 const ammo_to_cost_map_koef = 3;
class next_weapon_searcher
{
public:
	typedef xr_set<PIItem> exception_items_t;
	next_weapon_searcher(priority_group & pg,
						 PIItem & best_fit,
						 exception_items_t & except_set,
						 bool ignore_ammo) : 
		m_prior_group(pg),
		m_best_fit(best_fit),
		m_best_fit_cost(0),
		m_best_fit_ammo_elapsed(0),
		m_except_set(except_set),
		m_ignore_ammo(ignore_ammo)
	{
		m_best_fit = NULL;
	};

	next_weapon_searcher(next_weapon_searcher const & copy) :
		m_prior_group(copy.m_prior_group),
		m_best_fit(copy.m_best_fit),
		m_best_fit_cost(copy.m_best_fit_cost),
		m_best_fit_ammo_elapsed(copy.m_best_fit_ammo_elapsed),
		m_except_set(copy.m_except_set),
		m_ignore_ammo(copy.m_ignore_ammo)
	{
	};

	void operator() (PIItem const & item)
	{
		if (!item)
			return;
		
		CGameObject* tmp_obj = item->cast_game_object();
		if (!tmp_obj)
			return;

		if (m_except_set.find(item) != m_except_set.end())
			return;

		if (!m_prior_group.is_item_in_group(tmp_obj->cNameSect()))
			return;

		CWeapon* tmp_weapon = smart_cast<CWeapon*>(tmp_obj);
		if (!tmp_weapon)
			return;

		u32 tmp_ammo_count	= tmp_weapon->GetAmmoElapsed();
		u32 tmp_cost		= tmp_weapon->CInventoryItem::Cost();

		if (!tmp_ammo_count && !m_ignore_ammo)
			return;
		
		if (!m_best_fit)
		{
			m_best_fit				= item;
			m_best_fit_cost			= tmp_weapon->CInventoryItem::Cost();
			m_best_fit_ammo_elapsed	= tmp_weapon->GetAmmoElapsed();
			return;
		}
		
		if ((tmp_cost + (tmp_ammo_count*ammo_to_cost_map_koef)) >
			(m_best_fit_cost + (m_best_fit_ammo_elapsed*ammo_to_cost_map_koef)))
		{
			m_best_fit				= item;
			m_best_fit_cost			= tmp_weapon->CInventoryItem::Cost();
			m_best_fit_ammo_elapsed	= tmp_weapon->GetAmmoElapsed();
			return;
		}
	};
private:
	next_weapon_searcher & operator = (next_weapon_searcher const & copy) {}
	priority_group &	m_prior_group;
	exception_items_t &	m_except_set;
	PIItem &			m_best_fit;
	u32					m_best_fit_cost;
	u32					m_best_fit_ammo_elapsed;
	bool				m_ignore_ammo;
};// class next_weapon_searcher

priority_group & CInventory::GetPriorityGroup	(u8 const priority_value, u16 slot)
{
	R_ASSERT(priority_value < qs_priorities_count);
	if (slot == PISTOL_SLOT)
	{
		VERIFY(m_slot2_priorities[priority_value]);
		return *m_slot2_priorities[priority_value];
	}
	else if (slot == PRIMARY_SLOT)
	{
		VERIFY(m_slot3_priorities[priority_value]);
		return *m_slot3_priorities[priority_value];
	}
	return m_null_priority;
}

enum enum_priorities_groups
{
	epg_pistols			= 0x00,
	epg_shotgun,
	epg_assault,
	epg_sniper_rifels,
	epg_heavy_weapons,
	epg_groups_count
};

char const * groups_names[CInventory::qs_priorities_count] = 
{
	"pistols",
	"shotgun",
	"assault",
	"sniper_rifles",
	"heavy_weapons"
};

u32 g_slot2_pistol_switch_priority	= 0;
u32 g_slot2_shotgun_switch_priority = 1;
u32 g_slot2_assault_switch_priority = 2;
u32 g_slot2_sniper_switch_priority	= 4;
u32 g_slot2_heavy_switch_priority	= 3;

u32 g_slot3_pistol_switch_priority	= 4;	//not switch
u32 g_slot3_shotgun_switch_priority = 2;
u32 g_slot3_assault_switch_priority = 0;
u32 g_slot3_sniper_switch_priority	= 1;
u32 g_slot3_heavy_switch_priority	= 3;

static char const * teamdata_section	= "deathmatch_team0";

void CInventory::InitPriorityGroupsForQSwitch()
{
	STATIC_CHECK(epg_groups_count == CInventory::qs_priorities_count, groups_count_problem);
	for (int i = epg_pistols; i < epg_groups_count; ++i)
	{
		m_groups[i].init_group		(teamdata_section, groups_names[i]);
	}
	
	m_slot2_priorities[g_slot2_pistol_switch_priority]	= &m_groups[epg_pistols];
	m_slot2_priorities[g_slot2_shotgun_switch_priority] = &m_groups[epg_shotgun];
	m_slot2_priorities[g_slot2_assault_switch_priority]	= &m_groups[epg_assault];
	m_slot2_priorities[g_slot2_sniper_switch_priority]	= &m_groups[epg_sniper_rifels];
	m_slot2_priorities[g_slot2_heavy_switch_priority]	= &m_groups[epg_heavy_weapons];

	m_slot3_priorities[g_slot3_pistol_switch_priority]	= &m_groups[epg_assault];//&m_groups[epg_pistols];
	m_slot3_priorities[g_slot3_shotgun_switch_priority] = &m_groups[epg_shotgun];
	m_slot3_priorities[g_slot3_assault_switch_priority]	= &m_groups[epg_assault];
	m_slot3_priorities[g_slot3_sniper_switch_priority]	= &m_groups[epg_sniper_rifels];
	m_slot3_priorities[g_slot3_heavy_switch_priority]	= &m_groups[epg_heavy_weapons];
}

priority_group::priority_group()
{
}

void priority_group::init_group			(shared_str const & game_section, shared_str const & line)
{
	shared_str tmp_string = pSettings->r_string(game_section, line.c_str());
	string256	dststr;
	u32 count	= _GetItemCount(tmp_string.c_str());
	for (u32 i = 0; i < count; ++i)
	{
		_GetItem			(tmp_string.c_str(), i, dststr);
		m_sections.insert	(shared_str(dststr));
	}
}

bool priority_group::is_item_in_group	(shared_str const & section_name) const
{
	return m_sections.find(section_name) != m_sections.end();
}

////////////////////////////////////

void CInventory::ActivateDeffered()
{
	m_change_after_deactivate = true;
	Activate(NO_ACTIVE_SLOT);
}

PIItem CInventory::GetNextActiveGrenade()
{
	// (c) NanoBot
	xr_vector<shared_str> types_sect_grn;        // ������� ������ ������ ������
	// ������� ������ ������ ������ ������ ����� � ������
	// � m_belt ��� m_ruck ��� ������� ������� ����� ������ � �����, �.�. this
	types_sect_grn.push_back(ActiveItem()->cast_game_object()->cNameSect());
	int count_types = 1;    // ������� ���������� ����� ������ � ������
	TIItemContainer::iterator it = m_ruck.begin();
	TIItemContainer::iterator it_e = m_ruck.end();
	for (; it != it_e; ++it)
	{
		CGrenade *pGrenade = smart_cast<CGrenade*>(*it);
		if (pGrenade)
		{
			// ���������� ������ ����� ������ (�) �������
			xr_vector<shared_str>::const_iterator I = types_sect_grn.begin();
			xr_vector<shared_str>::const_iterator E = types_sect_grn.end();
			bool new_type = true;
			for (; I != E; ++I)
			{
				if (!xr_strcmp(pGrenade->cNameSect(), *I)) // ���� ���������
					new_type = false;
			}
			if (new_type)    // ����� ��� �������?, ���������
			{
				types_sect_grn.push_back(pGrenade->cNameSect());
				count_types++;
			}
		}
	}
	// ���� ����� ������ 1 ��, ��������� ������ �� ��������
	// � ������� ����� ������� ������� � ������.
	if (count_types > 1)
	{
		int curr_num = 0;        // ����� ���� ������� �������
		types_sect_grn.sort();
		xr_vector<shared_str>::const_iterator I = types_sect_grn.begin();
		xr_vector<shared_str>::const_iterator E = types_sect_grn.end();
		for (; I != E; ++I)
		{
			if (!xr_strcmp(ActiveItem()->cast_game_object()->cNameSect(), *I)) // ���� ���������
				break;
			curr_num++;
		}
		int next_num = curr_num + 1;    // ����� ������ ��������� �������
		if (next_num >= count_types)
			next_num = 0;

		shared_str sect_next_grn = types_sect_grn[next_num];    // ������ �������� �������
		// ���� � ������ ������� � ������� ��������� ����
		it = m_ruck.begin();
		it_e = m_ruck.end();
		for (; it != it_e; ++it)
		{
			CGrenade *pGrenade = smart_cast<CGrenade*>(*it);
			if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn))
				return *it;
		}
	}

	return nullptr;
}

bool CInventory::ActivateNextGrenage()
{
	if (m_iActiveSlot == NO_ACTIVE_SLOT)
		return false;

	CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);
	if (Level().CurrentViewEntity() != pActor_owner)
		return false;

	PIItem new_item = GetNextActiveGrenade();
	if (!new_item)
		return false;

	PIItem current_item = ActiveItem();
	if (current_item)
	{
		m_change_after_deactivate = false;
		Ruck(current_item);
		Slot(new_item->BaseSlot(), new_item);
	}
	return true;
}
