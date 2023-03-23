////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.cpp
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "purchase_list.h"
#include "inventoryowner.h"
#include "ai\trader\ai_trader.h"

#define CHECK_SECTION_VALID(section) !!xr_strcmp(READ_IF_EXISTS(pSettings, r_string, section, "inv_name", "default"), "default")

CItems ITEMS;

CItems::CItems()
{
	data.clear							();
	const CInifile::Root sections		= pSettings->sections();
	for (CInifile::Root::const_iterator it = sections.begin(), it_e = sections.end(); it != it_e; ++it)
	{
		shared_str section				= (*it)->Name;
		if (!CHECK_SECTION_VALID(*section) || READ_IF_EXISTS(pSettings, r_bool, section, "pseudosection", false))
			continue;

		shared_str main_class			= READ_IF_EXISTS(pSettings, r_string, section, "main_class", "nil");
		shared_str subclass				= READ_IF_EXISTS(pSettings, r_string, section, "subclass", "nil");
		shared_str division				= READ_IF_EXISTS(pSettings, r_string, section, "division", "nil");

		MAINCLASS MainClass				= data.find(main_class);
		if (MainClass == data.end())
		{
			SUBCLASSES					tmp;
			tmp.clear					();
			data.insert					(std::make_pair(main_class, tmp));
			MainClass					= data.find(main_class);
		}
		SUBCLASS Subclass				= MainClass->second.find(subclass);
		if (Subclass == MainClass->second.end())
		{
			DIVISIONS					tmp;
			tmp.clear					();
			MainClass->second.insert	(std::make_pair(subclass, tmp));
			Subclass					= MainClass->second.find(subclass);
		}
		DIVISION Division				= Subclass->second.find(division);
		if (Division == Subclass->second.end())
		{
			SECTIONS					tmp;
			tmp.clear					();
			Subclass->second.insert		(std::make_pair(division, tmp));
			Division					= Subclass->second.find(division);
		}
		Division->second.push_back		(section);
	}
}

CItems::~CItems()
{
}

MAINCLASS CItems::Get(LPCSTR main_class)
{
	return data.find(main_class);
}

SUBCLASS CItems::Get(LPCSTR main_class, LPCSTR subclass)
{
	MAINCLASS mc	= Get(main_class);
	return			mc->second.find		(subclass);
}

DIVISION CItems::Get(LPCSTR main_class, LPCSTR subclass, LPCSTR division)
{
	SUBCLASS sc		= Get(main_class, subclass);
	return			sc->second.find(division);
}

void CPurchaseList::process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner)
{
	owner.sell_useless_items		();
	CAI_Trader& trader				= smart_cast<CAI_Trader&>(owner);
	trader.supplies_list.clear_not_free();
	CInifile::Sect& S				= ini_file.r_section(section);
	CRandom random					((u32)(CPU::QPC() & u32(-1)));
	for (CInifile::SectCIt I = S.Data.begin(), I_e = S.Data.end(); I != I_e; ++I)
	{
		LPCSTR line					= *I->first;

		if (CHECK_SECTION_VALID(line))
		{
			GiveObject				(owner, line);
			continue;
		}

		string256					tmp0, tmp1, tmp2;
		shared_str main_class		= _GetItem(line, 0, tmp0, '.');
		int count					= _GetItemCount(line, '.');
		shared_str subclass			= (count > 1) ? _GetItem(line, 1, tmp1, '.') : "any";
		shared_str division			= (count > 2) ? _GetItem(line, 2, tmp2, '.') : "any";

		MAINCLASS MainClass			= ITEMS.Get(*main_class);
		for (SUBCLASS Subclass = MainClass->second.begin(), Subclass_e = MainClass->second.end(); Subclass != Subclass_e; ++Subclass)
		{
			if (subclass != "any" && Subclass->first != subclass)
				continue;
			for (DIVISION Division = Subclass->second.begin(), Division_e = Subclass->second.end(); Division != Division_e; ++Division)
			{
				if (division != "any" && Division->first != division)
					continue;
				for (SECTION Section = Division->second.begin(), Section_e = Division->second.end(); Section != Section_e; ++Section)
					GiveObject		(owner, **Section);
			}
		}
	}
	owner.inventory().InvalidateState();
}

void CPurchaseList::GiveObject(CInventoryOwner& owner, LPCSTR section)
{
	CAI_Trader& trader					= smart_cast<CAI_Trader&>(owner);
	shared_str							str;
	str._set							(section);
	trader.supplies_list.push_back		(str);
}