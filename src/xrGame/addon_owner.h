#pragma once
#include "module.h"

class CAddon;
class CAddonOwner;
class CAddonSlot;

typedef ::std::vector<xptr<CAddonSlot>> VSlots;

class CAddonSlot
{
public:
										CAddonSlot								(shared_str CR$ section, u16 _idx, CAddonOwner PC$ parent);

private:
	double								m_step;
	u16									m_overlaping_slot;
	bool								m_background_draw;
	bool								m_has_loading_anim;

	CAddon*								m_loading_addon							= nullptr;
	u16									m_bone_id								= u16_max;
	Dmatrix								m_bone_offset							= Didentity;

	int									get_spacing							C$	(CAddon CPC left, CAddon CPC right);
	CAddon*								get_next_addon						C$	(xr_list<CAddon*>::iterator& I);
	CAddon*								get_prev_addon						C$	(xr_list<CAddon*>::iterator& I);
	void								append_bone_trans					C$	(Dmatrix& trans, IKinematics* model);

public:
	static bool							isCompatible							(shared_str CR$ slot_type, shared_str CR$ addon_type);
	static LPCSTR						getSlotName								(LPCSTR slot_type);

	CAddonOwner PC$						parent_ao;
	const u16							idx;

	shared_str							type;
	shared_str							name;
	int									steps;
	Dmatrix								model_offset;
	Fvector2							icon_offset;
	float								icon_step;
	u8									blocking_iron_sights;		//1 for blocking if non-lowered addon attached, 2 for force block on any addon
	shared_str							attach;

	xr_list<CAddon*>					addons									= {};

	void								attachAddon								(CAddon* addon);
	void								detachAddon								(CAddon* addon);
	void								shiftAddon								(CAddon* addon, int shift);

	void								updateAddonsHudTransform				(IKinematics* model, Dmatrix CR$ parent_trans, Dvector CR$ root_offset);

	void								startReloading							(CAddon* loading_addon);
	void								loadingDetach							();
	void								loadingAttach							();
	void								finishLoading							(bool interrupted = false);
	void								calculateBoneOffset						(IKinematics* model, shared_str CR$ hud_sect);
	
	bool								backgroundDraw						C$	()		{ return m_background_draw || m_has_loading_anim; }
	bool								isLoading							C$	()		{ return !!m_loading_addon; }
	CAddon CP$							getLoadingAddon						C$	()		{ return m_loading_addon; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	bool								CanTake								C$	(CAddon CPC addon);
	void								updateAddonLocalTransform			C$	(CAddon* addon);
};

class CAddonOwner : public CModule
{
public:
										CAddonOwner								(CGameObject* obj);

private:
	bool								m_base_foreground_draw;

	VSlots								m_slots									= {};
	
	void								transfer_addon							(CAddon CPC addon, bool attach);
	void								processAddon						C$	(CAddon PC$ addon, bool attach);
	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_slots; }
	bool								getBaseForegroundDraw				C$	()		{ return m_base_foreground_draw; }

	CAddonOwner*						getParentAO							C$	();
	CAddonSlot*							findAvailableSlot					C$	(CAddon CPC addon);
	void								RegisterAddon						C$	(CAddon PC$ addon, bool attach);

	bool								attachAddon								(CAddon* addon);
	void								detachAddon								(CAddon* addon);
	void								calculateSlotsBoneOffset				(IKinematics* model, shared_str CR$ hud_sect);

	bool							S$	LoadAddonSlots							(shared_str CR$ section, VSlots& slots, CAddonOwner PC$ parent_ao = nullptr);
};
