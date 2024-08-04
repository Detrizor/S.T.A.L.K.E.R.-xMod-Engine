#pragma once
#include "module.h"

class MAddon;
class MAddonOwner;
class CAddonSlot;

typedef ::std::vector<xptr<CAddonSlot>> VSlots;

class CAddonSlot
{
public:
										CAddonSlot								(shared_str CR$ section, u16 _idx, MAddonOwner PC$ parent);

private:
	double								m_step;
	u16									m_overlaping_slot;
	bool								m_background_draw;
	bool								m_foreground_draw;

	MAddon*								m_loading_addon							= nullptr;
	u16									m_bone_id								= u16_max;
	Dmatrix								m_bone_offset							= Didentity;

	int									get_spacing							C$	(MAddon CPC left, MAddon CPC right);
	MAddon*								get_next_addon						C$	(xr_list<MAddon*>::iterator& I);
	MAddon*								get_prev_addon						C$	(xr_list<MAddon*>::iterator& I);
	void								append_bone_trans					C$	(Dmatrix& trans, IKinematics* model);

public:
	static bool							isCompatible							(shared_str CR$ slot_type, shared_str CR$ addon_type);
	static LPCSTR						getSlotName								(LPCSTR slot_type);

	MAddonOwner PC$						parent_ao;
	const u16							idx;

	shared_str							type;
	shared_str							name;
	int									steps;
	Dmatrix								model_offset;
	Fvector2							icon_offset;
	float								icon_step;
	shared_str							attach;

	xr_list<MAddon*>					addons									= {};

	void								attachAddon								(MAddon* addon);
	void								detachAddon								(MAddon* addon);
	void								shiftAddon								(MAddon* addon, int shift);

	void								updateAddonsHudTransform				(IKinematics* model, Dmatrix CR$ parent_trans, Dvector CR$ root_offset);

	void								startReloading							(MAddon* loading_addon);
	void								loadingDetach							();
	void								loadingAttach							();
	void								finishLoading							(bool interrupted = false);
	void								calculateBoneOffset						(IKinematics* model, shared_str CR$ hud_sect);
	
	bool								getBackgroundDraw					C$	()		{ return m_background_draw; }
	bool								getForegroundDraw					C$	()		{ return m_foreground_draw; }
	bool								isLoading							C$	()		{ return !!m_loading_addon; }
	MAddon CP$							getLoadingAddon						C$	()		{ return m_loading_addon; }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	bool								CanTake								C$	(MAddon CPC addon);
	void								updateAddonLocalTransform			C$	(MAddon* addon);
};

class MAddonOwner : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mAddonOwner; }
	
public:
										MAddonOwner								(CGameObject* obj);

private:
	bool								m_base_foreground_draw;

	VSlots								m_slots									= {};
	
	void								transfer_addon							(MAddon CPC addon, bool attach);
	void								processAddon						C$	(MAddon PC$ addon, bool attach);
	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	VSlots CR$							AddonSlots							C$	()		{ return m_slots; }
	bool								getBaseForegroundDraw				C$	()		{ return m_base_foreground_draw; }

	MAddonOwner*						getParentAO							C$	();
	CAddonSlot*							findAvailableSlot					C$	(MAddon CPC addon);
	void								RegisterAddon						C$	(MAddon PC$ addon, bool attach);

	bool								attachAddon								(MAddon* addon);
	void								detachAddon								(MAddon* addon);
	void								calculateSlotsBoneOffset				(IKinematics* model, shared_str CR$ hud_sect);

	bool							S$	LoadAddonSlots							(shared_str CR$ section, VSlots& slots, MAddonOwner PC$ parent_ao = nullptr);
};
