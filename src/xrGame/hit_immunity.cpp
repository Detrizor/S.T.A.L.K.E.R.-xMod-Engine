// hit_immunity.cpp:	класс для тех объектов, которые поддерживают
//						коэффициенты иммунитета для разных типов хитов
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "hit_immunity.h"

CHitImmunity::CHitImmunity()
{
	m_HitImmunityKoefs.resize	(ALife::eHitTypeMax);
	for (int i = 0; i < ALife::eHitTypeMax; i++)
		m_HitImmunityKoefs[i]	= 1.0f;
}

CHitImmunity::~CHitImmunity()
{
}
void CHitImmunity::LoadImmunities(LPCSTR imm_sect, CInifile const * ini)
{
	R_ASSERT2	(ini->section_exist(imm_sect), imm_sect);

	m_HitImmunityKoefs[ALife::eHitTypeBurn]				= READ_IF_EXISTS(ini, r_float, imm_sect, "burn_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeStrike]			= READ_IF_EXISTS(ini, r_float, imm_sect, "strike_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeShock]			= READ_IF_EXISTS(ini, r_float, imm_sect, "shock_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeWound]			= READ_IF_EXISTS(ini, r_float, imm_sect, "wound_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeRadiation]		= READ_IF_EXISTS(ini, r_float, imm_sect, "radiation_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeTelepatic]		= READ_IF_EXISTS(ini, r_float, imm_sect, "telepatic_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeChemicalBurn]		= READ_IF_EXISTS(ini, r_float, imm_sect, "chemical_burn_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeExplosion]		= READ_IF_EXISTS(ini, r_float, imm_sect, "explosion_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeFireWound]		= READ_IF_EXISTS(ini, r_float, imm_sect, "fire_wound_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeLightBurn]		= m_HitImmunityKoefs[ALife::eHitTypeBurn];
}

void CHitImmunity::AddImmunities(LPCSTR imm_sect, CInifile const * ini)
{
	R_ASSERT2	(ini->section_exist(imm_sect), imm_sect);

	m_HitImmunityKoefs[ALife::eHitTypeBurn]				+= READ_IF_EXISTS(ini, r_float, imm_sect,"burn_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeStrike]			+= READ_IF_EXISTS(ini, r_float, imm_sect,"strike_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeShock]			+= READ_IF_EXISTS(ini, r_float, imm_sect,"shock_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeWound]			+= READ_IF_EXISTS(ini, r_float, imm_sect,"wound_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeRadiation]		+= READ_IF_EXISTS(ini, r_float, imm_sect,"radiation_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeTelepatic]		+= READ_IF_EXISTS(ini, r_float, imm_sect,"telepatic_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeChemicalBurn]		+= READ_IF_EXISTS(ini, r_float, imm_sect,"chemical_burn_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeExplosion]		+= READ_IF_EXISTS(ini, r_float, imm_sect,"explosion_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeFireWound]		+= READ_IF_EXISTS(ini, r_float, imm_sect,"fire_wound_immunity", 0.f);
	m_HitImmunityKoefs[ALife::eHitTypeLightBurn]		= m_HitImmunityKoefs[ALife::eHitTypeBurn];
}