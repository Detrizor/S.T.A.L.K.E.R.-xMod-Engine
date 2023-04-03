#include "stdafx.h"
#include "silencer.h"

CSilencer::CSilencer(CGameObject* obj, shared_str CR$ section) : CModule(obj)
{
	m_section							= section;
}
