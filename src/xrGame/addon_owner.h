#pragma once
#include "module.h"

class CAddon;
class CAddonOwner;
class CAddonSlot;

typedef xr_vector<CAddonSlot*> VSlots;

class CAddonSlot
{
public:
										CAddonSlot								(LPCSTR section, u16 _idx, CAddonOwner PC$ parent);

private:
	CAddon CPC							m_parent_addon;

	float								m_step;
	u16									m_overlaping_slot;
	BOOL								m_has_loading_anim;
	BOOL								m_slide_attach;

	CAddon*								m_loading_addon							= nullptr;
	u16									m_bone_id								= u16_max;
	Fmatrix								m_bone_offset							= Fidentity;

	int									get_spacing							C$	(CAddon CPC left, CAddon CPC right);
	CAddon*								get_next_addon						C$	(xr_list<CAddon*>::iterator& I);
	CAddon*								get_prev_addon						C$	(xr_list<CAddon*>::iterator& I);
	void								append_bone_trans					C$	(Fmatrix& trans, IKinematics* model);

public:
	CAddonOwner PC$						parent_ao;
	const u16							idx;

	shared_str							name;
	shared_str							type;
	int									steps;
	Fmatrix								model_offset;
	Fvector2							icon_offset;
	float								icon_step;
	u8									blocking_iron_sights;		//1 for blocking if non-lowered addon attached, 2 for force block on any addon
	bool								muzzle;

	xr_list<CAddon*>					addons									= {};

	void								attachAddon								(CAddon* addon);
	void								detachAddon								(CAddon* addon);
	void								shiftAddon								(CAddon* addon, int shift);

	void								updateAddonsHudTransform				(IKinematics* model, Fmatrix CR$ parent_trans);
	void								updateAddonsHudTransform				(Fmatrix CR$ parent_trans);

	void								startReloading							(CAddon* loading_addon);
	void								loadingDetach							();
	void								loadingAttach							();
	void								finishLoading							(bool interrupted = false);
	void								calculateBoneOffset						(IKinematics* model);
	
	bool								hasLoadingAnim						C$	()		{ return m_has_loading_anim; }
	bool								isLoading							C$	()		{ return !!m_loading_addon; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	bool								Compatible							C$	(CAddon CPC addon);
	bool								CanTake								C$	(CAddon CPC addon);
	void								updateAddonLocalTransform			C$	(CAddon* addon);
};

class CAddonOwner : public CModule
{
public:
										CAddonOwner								(CGameObject* obj);
										~CAddonOwner							();

private:
	VSlots								m_Slots;
	
	void								LoadAddonSlots							(LPCSTR section);
	int									TransferAddon							(CAddon CPC addon, bool attach);
	
	CAddonSlot*							find_available_slot					C$	(CAddon* addon);

	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_Slots; }

	CAddonOwner*						getParentAO							C$	();

	int									AttachAddon								(CAddon* addon, CAddonSlot* slot = NULL);
	int									DetachAddon								(CAddon* addon);
	void								RegisterAddon							(CAddon PC$ addon, bool attach);
	void								calculateSlotsBoneOffset				();

	void							S$	LoadAddonSlots							(LPCSTR section, VSlots& slots, CAddonOwner PC$ parent_ao = NULL);
};
