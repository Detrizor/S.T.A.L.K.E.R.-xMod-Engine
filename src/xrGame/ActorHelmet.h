#pragma once

#include "inventory_item_object.h"

struct SBoneProtections;

class CHelmet : public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;

public:
							CHelmet					();
	virtual					~CHelmet				();

	virtual void			Load					(LPCSTR section);
	
	virtual void			Hit						(float P, ALife::EHitType hit_type);

	shared_str				m_BonesProtectionSect;
	shared_str				m_NightVisionSect;

	virtual void			OnMoveToSlot			(const SInvItemPlace& previous_place);
	virtual BOOL			net_Spawn				(CSE_Abstract* DC);

	float					GetHitTypeProtection	(ALife::EHitType hit_type);
	float					GetBoneArmor			(s16 element);
	float					GetBoneArmorLevel		(s16 element);

	float					m_fShowNearestEnemiesDistance;
	float					m_fRecuperationFactor;
	float					m_fArmorLevel;

	void					ReloadBonesProtection	();
	void					AddBonesProtection		(LPCSTR bones_section, float value);

protected:
	HitImmunity::HitTypeSVec	m_HitTypeProtection;
	SBoneProtections*		m_boneProtection;	

protected:
	virtual bool			install_upgrade_impl	( LPCSTR section, bool test );
	
protected:
	xoptional<float>					sGetBar								O$	()		{ return (fLess(GetCondition(), 1.f)) ? GetCondition() : -1.f; }

private:
	float								m_fHealth;

public:
	float								Health								C$	()		{ return m_fHealth; }
};
