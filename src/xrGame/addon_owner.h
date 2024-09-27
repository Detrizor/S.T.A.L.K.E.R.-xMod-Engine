#pragma once
#include "module.h"

class MAddon;
class MAddonOwner;
class CAddonSlot;
struct attachable_hud_item;

typedef xr_vector<xptr<CAddonSlot>> VSlots;

class CAddonSlot
{
	typedef xr_umap<LPCSTR, RStringVec>	exceptions_list;

public:
										CAddonSlot								(u16 _idx, MAddonOwner PC$ _parent_ao);
	void								load									(shared_str CR$ section, shared_str CR$ parent_section);

private:
	static exceptions_list				slot_exceptions;
	static exceptions_list				addon_exceptions;

	u16									m_overlaping_slot						= u16_max;
	bool								m_background_draw						= false;
	bool								m_foreground_draw						= false;
	shared_str							m_attach_bone							= "root";

	MAddon*								m_loading_addon							= nullptr;
	u16									m_attach_bone_id						= 0;
	Dmatrix								m_attach_bone_offset					= Didentity;

	int									get_spacing							C$	(MAddon CPC left, MAddon CPC right);
	MAddon*								get_next_addon						C$	(xr_list<MAddon*>::iterator& I);
	MAddon*								get_prev_addon						C$	(xr_list<MAddon*>::iterator& I);
	void								update_addon_local_transform		C$	(MAddon* addon);

public:
	static bool							isCompatible							(shared_str CR$ slot_type, shared_str CR$ addon_type);
	static LPCSTR						getSlotName								(LPCSTR slot_type);
	static void							loadStaticData							();
	
	const u16							idx;
	MAddonOwner PC$						parent_ao;

	shared_str							type									= 0;
	shared_str							name									= 0;

	double								step									= 0.;
	int									steps									= 0;
	Dmatrix								model_offset							= Didentity;
	Fvector2							icon_offset								= vZero2;
	float								icon_step								= 0.f;

	xr_list<MAddon*>					addons									= {};

	void								attachAddon								(MAddon* addon);
	void								detachAddon								(MAddon* addon);
	void								shiftAddon								(MAddon* addon, int shift);

	void								updateAddonsHudTransform				(attachable_hud_item* hi);

	void								startReloading							(MAddon* loading_addon);
	void								loadingDetach							();
	void								loadingAttach							();
	void								finishLoading							(bool interrupted = false);
	void								calcBoneOffset							(attachable_hud_item* hi);
	
	bool								getBackgroundDraw					C$	()		{ return m_background_draw; }
	bool								getForegroundDraw					C$	()		{ return m_foreground_draw; }
	bool								isLoading							C$	()		{ return !!m_loading_addon; }
	MAddon CP$							getLoadingAddon						C$	()		{ return m_loading_addon; }
	shared_str CR$						getAttachBone						C$	()		{ return m_attach_bone; }
	bool								empty								C$	()		{ return addons.empty(); }

	void								RenderHud							C$	();
	void								RenderWorld							C$	(Fmatrix CR$ parent_trans);
	bool								canTake								C$	(MAddon CPC addon, bool forced = false);
	void								updateAddonLocalTransform			C$	(MAddon* addon, Dmatrix CPC parent_transform = nullptr);
};

class MAddonOwner : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mAddonOwner; }
	
public:
										MAddonOwner								(CGameObject* obj);

private:
	static bool							try_transfer							(MAddonOwner* ao, void* addon, int attach);

	bool								m_base_foreground_draw;

	VSlots								m_slots									= {};
	Dmatrix								m_root_offset							= Didentity;
	
	void								transfer_addon							(MAddon* addon, bool attach);
	void								detach_addon							(MAddon* addon);
	void								processAddon						C$	(MAddon PC$ addon, bool attach, bool recurrent = false);
	float								aboba								O$	(EEventTypes type, void* data, int param);
	
public:
	static bool							loadAddonSlots							(shared_str CR$ section, VSlots& slots, MAddonOwner* ao = nullptr);

	bool								attachAddon								(MAddon* addon, bool forced);
	void								detachAddon								(MAddon* addon);
	void								calcSlotsBoneOffset						(attachable_hud_item* hi);

	VSlots CR$							AddonSlots							C$	()		{ return m_slots; }
	bool								getBaseForegroundDraw				C$	()		{ return m_base_foreground_draw; }
	Dmatrix CR$							getRootOffset						C$	()		{ return m_root_offset; }

	MAddonOwner*						getParentAO							C$	();
	CAddonSlot*							findAvailableSlot					C$	(MAddon CPC addon, bool forced = false);
	void								RegisterAddon						C$	(MAddon PC$ addon, bool attach);
};
