#include "pch_script.h"
#include "ActorCondition.h"
#include "EntityCondition.h"

using namespace luabind;

#pragma optimize("s",on)
void CActorCondition::script_register(lua_State *L)
{
	module(L)
		[
			class_<CEntityCondition>("CEntityCondition")
			//.def(constructor<>())
			.def("GetWhoHitLastTimeID", &CEntityCondition::GetWhoHitLastTimeID)
			.def("GetPower", &CEntityCondition::GetPower)
			.def("GetRadiation", &CEntityCondition::GetRadiation)
			.def("GetPsyHealth", &CEntityCondition::GetPsyHealth)
			.def("GetSatiety", &CEntityCondition::GetSatiety)
			.def("GetEntityMorale", &CEntityCondition::GetEntityMorale)
			.def("GetHealthLost", &CEntityCondition::GetHealthLost)
			.def("IsLimping", &CEntityCondition::IsLimping)
			.def("ChangeSatiety", &CEntityCondition::ChangeSatiety)
			.def("ChangeHealth", &CEntityCondition::ChangeHealth)
			.def("ChangePower", &CEntityCondition::ChangePower)
			.def("ChangeRadiation", &CEntityCondition::ChangeRadiation)
			.def("ChangePsyHealth", &CEntityCondition::ChangePsyHealth)
			.def("ChangeAlcohol", &CEntityCondition::ChangeAlcohol)
			.def("SetMaxPower", &CEntityCondition::SetMaxPower)
			.def("GetMaxPower", &CEntityCondition::GetMaxPower)
			.def("ChangeEntityMorale", &CEntityCondition::ChangeEntityMorale)
			.def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
			.def("BleedingSpeed", &CEntityCondition::BleedingSpeed)
			.def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
			.def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
			.def("ChangeBleeding", &CEntityCondition::ChangeBleeding),

			class_<CActorCondition, CEntityCondition>("CActorCondition")
			//.def(constructor<>())
			.def("V_Satiety", &CActorCondition::V_Satiety)
			.def("V_SatietyPower", &CActorCondition::V_SatietyPower)
			.def("V_SatietyHealth", &CActorCondition::V_SatietyHealth)
			.def("SatietyCritical", &CActorCondition::SatietyCritical)
			.def("IsLimping", &CActorCondition::IsLimping)
			.def("IsCantWalk", &CActorCondition::IsCantWalk)
			.def("IsCantWalkWeight", &CActorCondition::IsCantWalkWeight)
			.def("IsCantSprint", &CActorCondition::IsCantSprint)
			.def_readwrite("m_fJumpPower", &CActorCondition::m_fJumpPower)
			.def_readwrite("m_fStandPower", &CActorCondition::m_fStandPower)
			.def_readwrite("m_fJumpWeightPower", &CActorCondition::m_fJumpWeightPower)
			.def_readwrite("m_fWalkWeightPower", &CActorCondition::m_fWalkWeightPower)
			.def_readwrite("m_fOverweightWalkK", &CActorCondition::m_fOverweightWalkK)
			.def_readwrite("m_fOverweightJumpK", &CActorCondition::m_fOverweightJumpK)
			.def_readwrite("m_fAccelK", &CActorCondition::m_fAccelK)
			.def_readwrite("m_fSprintK", &CActorCondition::m_fSprintK)
			.def_readwrite("m_condition_flags", &CActorCondition::m_condition_flags)
			.enum_("condition_flags")
			[
				value("eCriticalPowerReached", int(CActorCondition::eCriticalPowerReached)),
				value("eCriticalBleedingSpeed", int(CActorCondition::eCriticalBleedingSpeed)),
				value("eCriticalSatietyReached", int(CActorCondition::eCriticalSatietyReached)),
				value("eCriticalRadiationReached", int(CActorCondition::eCriticalRadiationReached)),
				value("eWeaponJammedReached", int(CActorCondition::eWeaponJammedReached)),
				value("ePhyHealthMinReached", int(CActorCondition::ePhyHealthMinReached)),
				value("ePhyHealthMinReached", int(CActorCondition::ePhyHealthMinReached)),
				value("eCantWalkWeight", int(CActorCondition::eCantWalkWeight)),
				value("eCantWalkWeightReached", int(CActorCondition::eCantWalkWeightReached))
			]
		];
}