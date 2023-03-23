#include "pch_script.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"

using namespace luabind;

#pragma optimize("s",on)
void CCustomOutfit::script_register(lua_State *L)
{
	module(L)
		[
			class_<CCustomOutfit, CGameObject>("CCustomOutfit")
			.def(constructor<>())
			.def_readwrite("m_fPowerLoss",			&CCustomOutfit::m_fPowerLoss)
			.def_readonly("bIsHelmetAvaliable",		&CCustomOutfit::bIsHelmetAvaliable)
			.def("get_artefact_count",				&CCustomOutfit::get_artefact_count),

			class_<CHelmet, CGameObject>("CHelmet")
			.def(constructor<>())
		];
}