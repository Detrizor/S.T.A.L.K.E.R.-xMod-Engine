#include "pch_script.h"
#include "Scope.h"
#include "Silencer.h"
#include "GrenadeLauncher.h"

CScope::CScope	()
{
	m_fScopeZoomFactor			= 1.f;
	m_fScopeMinZoomFactor		= 1.f;
	m_bScopeAliveDetector		= "";
}

CScope::~CScope	() 
{
}

void CScope::Load(LPCSTR section)
{
	inherited::Load(section);

	m_fScopeZoomFactor			= READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_factor", 1.f);
	m_fScopeMinZoomFactor		= READ_IF_EXISTS(pSettings, r_float, section, "scope_dynamic_zoom", m_fScopeZoomFactor);
	m_bScopeAliveDetector		= READ_IF_EXISTS(pSettings, r_string, section, "scope_alive_detector", NULL);
}

bool CScope::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result		= inherited::install_upgrade_impl(section, test);
	result			|= process_if_exists(section, "scope_zoom_factor", m_fScopeZoomFactor, test);
	result			|= process_if_exists(section, "scope_min_zoom_factor", m_fScopeMinZoomFactor, test);
	result			|= process_if_exists(section, "scope_alive_detector", m_bScopeAliveDetector, test);
	return			result;
}

using namespace luabind;

#pragma optimize("s",on)
void CScope::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CScope,CGameObject>("CScope")
			.def(constructor<>()),
		
		class_<CSilencer,CGameObject>("CSilencer")
			.def(constructor<>()),

		class_<CGrenadeLauncher,CGameObject>("CGrenadeLauncher")
			.def(constructor<>())
	];
}
