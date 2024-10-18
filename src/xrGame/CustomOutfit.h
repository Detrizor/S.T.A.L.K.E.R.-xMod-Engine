#pragma once

#include "inventory_item_object.h"
#include "script_export_space.h"

struct SBoneProtections;

class CCustomOutfit : public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;

public:
								CCustomOutfit			();
	virtual						~CCustomOutfit			();

	virtual void				Load					(LPCSTR section);
	
	//уменьшенная версия хита, для вызова, когда костюм надет на персонажа
	virtual void				Hit						(float P, ALife::EHitType hit_type);

	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет костюм
	float						GetHitTypeProtection	(ALife::EHitType hit_type);
	float						GetBoneArmor			(s16 element);
	float						GetBoneArmorLevel		(s16 element);

	virtual void				OnMoveToSlot			(const SInvItemPlace& prev);
	virtual void				OnMoveToRuck			(const SInvItemPlace& previous_place);

	void						GetPockets				(LPCSTR pockets);

protected:
	HitImmunity::HitTypeSVec	m_HitTypeProtection;
	SBoneProtections*			m_boneProtection;
	shared_str					m_ActorVisual;

	u32							m_ef_equipment_type;
	u32							m_artefact_count;

public:
	float						m_capacity;
	xr_vector<float>			m_pockets;

	float						m_fPowerLoss;
	float						m_fWeightDump;
	float						m_fRecuperationFactor;
	float						m_fDrainFactor;
	float						m_fArmorLevel;

	shared_str					m_BonesProtectionSect;
	shared_str					m_NightVisionSect;

	bool						bIsHelmetAvaliable;

	virtual u32					ef_equipment_type		() const;
	u32							get_artefact_count		() const	{ return m_artefact_count; }

	virtual BOOL				net_Spawn				(CSE_Abstract* DC);
			void				ApplySkinModel			(CActor* pActor, bool bDress, bool bHUDOnly);
			void				ReloadBonesProtection	();
			void				AddBonesProtection		(LPCSTR bones_section, float value);

protected:
	virtual bool				install_upgrade_impl	( LPCSTR section, bool test );

	DECLARE_SCRIPT_REGISTER_FUNCTION

protected:
	xoptional<float>					sGetBar								O$	()		{ return (fLess(GetCondition(), 1.f)) ? GetCondition() : -1.f; }

private:
	float								m_fHealth;

public:
	float								Health								C$	()		{ return m_fHealth; }
};

add_to_type_list(CCustomOutfit)
#undef script_type_list
#define script_type_list save_type_list(CCustomOutfit)
