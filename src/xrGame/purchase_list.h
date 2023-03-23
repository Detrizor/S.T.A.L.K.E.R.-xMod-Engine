////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.h
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "associative_vector.h"

class CInventoryOwner;
class CGameObject;

typedef xr_vector<shared_str>						SECTIONS;
typedef SECTIONS::iterator							SECTION;
typedef associative_vector<shared_str, SECTIONS>	DIVISIONS;
typedef DIVISIONS::iterator							DIVISION;
typedef associative_vector<shared_str, DIVISIONS>	SUBCLASSES;
typedef SUBCLASSES::iterator						SUBCLASS;
typedef associative_vector<shared_str, SUBCLASSES>	MAINCLASSES;
typedef MAINCLASSES::iterator						MAINCLASS;

class CItems
{
	MAINCLASSES		data;

public:
					CItems		();
					~CItems		();

	MAINCLASS		Get			(LPCSTR main_class);
	SUBCLASS		Get			(LPCSTR main_class, LPCSTR subclass);
	DIVISION		Get			(LPCSTR main_class, LPCSTR subclass, LPCSTR division);
};

class CPurchaseList {
public:
	typedef associative_vector<shared_str,float>	DEFICITS;

private:
	DEFICITS				m_deficits;

			void			GiveObject			(CInventoryOwner& owner, LPCSTR section);

public:
			void			process				(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner);

public:
	IC		void			deficit				(const shared_str& section, const float& deficit);
	IC		float			deficit				(const shared_str& section) const;
	IC		const DEFICITS&	deficits			() const;
};

#include "purchase_list_inline.h"