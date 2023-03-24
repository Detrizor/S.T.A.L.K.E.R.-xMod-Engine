#pragma once
#include "inventory_item.h"

class CInventory;
class CInventoryItem;
class CHudItem;
class CInventoryOwner;

class CInventorySlot
{									
public:
							CInventorySlot		();
	virtual					~CInventorySlot		();

	bool					CanBeActivated		() const;

	PIItem					m_pIItem;
	bool					m_bPersistent;
	bool					m_bAct;
};

class priority_group
{
public:
			priority_group		();
	void	init_group			(shared_str const & game_section, shared_str const & line);
	bool	is_item_in_group	(shared_str const & section_name) const;
private:
	xr_set<shared_str>			m_sections;
};//class priority_group


typedef xr_vector<CInventorySlot>		TISlotArr;
typedef xr_vector<TIItemContainer>		TIICArr;


class CInventory
{				
public:
							CInventory			();
	virtual					~CInventory			();

	float 					TotalWeight			() const;
	float 					TotalVolume			() const;
	float 					CalcTotalWeight		();
	float 					CalcTotalVolume		();

	void					Take				(CGameObject *pObj, bool strict_placement);
	//if just_before_destroy is true, then activate will be forced (because deactivate message will not deliver)
	bool					DropItem			(CGameObject *pObj, bool just_before_destroy, bool dont_create_shell);
	void					Clear				();

	u16 					m_last_slot;
	u8	 					m_pockets_count;
	IC u16					FirstSlot			() const {return 1;}
	IC u16					LastSlot			() const {return m_last_slot;} // not "end"
	IC bool					SlotIsPersistent	(u16 slot_id) const {return m_slots[slot_id].m_bPersistent;}
	bool					Slot				(u16 slot_id, PIItem pIItem);
	bool					Pocket				(PIItem pIItem, u16 pocket_id, bool forced = false);
	bool					Bag					(PIItem item);
	bool					Ruck				(PIItem pIItem, bool strict_placement=false);

	bool 					InSlot				(const CInventoryItem* pIItem) const;
	bool 					InRuck				(const CInventoryItem* pIItem) const;

	bool 					CanPutInSlot		(PIItem pIItem, u16 slot_id) const;
	bool 					CanPutInRuck		(PIItem pIItem) const;
	bool 					CanPutInPocket		(PIItem pIItem, u16 pocket_id) const;
	bool					PocketPresent		(u16 pocket_id) const;
	
	bool					ProcessItem			(PIItem item);
	void					EmptyPockets		();

	bool					CanTakeItem			(CInventoryItem *inventory_item) const;

	void					Activate			(u16 slot, bool bForce = false);
	void					ActivateItem		(PIItem item, u16 return_place = 0, u16 return_slot = 0);
	void					ActivateDeffered();
	PIItem					GetNextActiveGrenade();
	bool					ActivateNextGrenage();
	
	static u32 const		qs_priorities_count = 5;
	PIItem					GetNextItemInActiveSlot		(u8 const priority_value, bool ignore_ammo);
	bool					ActivateNextItemInActiveSlot();
	priority_group &		GetPriorityGroup			(u8 const priority_value, u16 slot);
	void					InitPriorityGroupsForQSwitch();

	PIItem					ActiveItem			() const;
	PIItem					LeftItem			(bool with_activation = false) const;
	bool					InHands				(PIItem item) const		{ return item == ActiveItem() || item == LeftItem(); }
	PIItem					ItemFromSlot		(u16 slot) const;

	bool					Action				(u16 cmd, u32 flags);
	void					Update				();
	// »щет на по€се аналогичный IItem
	PIItem					Same				(const PIItem pIItem) const;
	// »щет на по€се IItem дл€ указанного слота
	PIItem					SameSlot			(const u16 slot, PIItem pIItem) const;
	// »щет на по€се или в рюкзаке IItem с указанным именем (cName())
	PIItem					Get					(LPCSTR name, int pocket_id = -1, bool full = false) const;
	// »щет на по€се или в рюкзаке IItem с указанным именем (id)
	PIItem					Get					(const u16  id) const;
	// »щет на по€се или в рюкзаке IItem с указанным CLS_ID
	PIItem					Get					(CLASS_ID cls_id) const;
	PIItem					GetAny				(LPCSTR name) const;
	PIItem					item				(CLASS_ID cls_id) const;
	
	// get all the items with the same section name
	virtual u32				dwfGetSameItemCount	(LPCSTR caSection, bool SearchAll = false);	
	virtual u32				dwfGetGrenadeCount	(LPCSTR caSection, bool SearchAll);	
	// get all the items with the same object id
	virtual bool			bfCheckForObject	(ALife::_OBJECT_ID tObjectID);	
	PIItem					get_object_by_id	(ALife::_OBJECT_ID tObjectID);

	u32						dwfGetObjectCount	();
	PIItem					tpfGetObjectByIndex	(int iIndex);
	PIItem					GetItemFromInventory(LPCSTR caItemName);

	bool					Eat					(PIItem pIItem);
	bool					ClientEat			(PIItem pIItem);

	u16						GetActiveSlot		() const;
	
	void					SetPrevActiveSlot	(u16 ActiveSlot)	{m_iPrevActiveSlot = ActiveSlot;}
	u16						GetPrevActiveSlot	() const			{return m_iPrevActiveSlot;}
	IC u16					GetNextActiveSlot	() const			{return m_iNextActiveSlot;}

	void					SetActiveSlot		(u16 ActiveSlot)	{m_iActiveSlot = m_iNextActiveSlot = ActiveSlot; }

	bool 					IsSlotsUseful		() const			{return m_bSlotsUseful;}	 
	void 					SetSlotsUseful		(bool slots_useful) {m_bSlotsUseful = slots_useful;}

	void					SetSlotsBlocked		(u16 mask, bool bBlock);
	
	void					BlockSlot(u16 slot_id);
	void					UnblockSlot(u16 slot_id);
	bool					IsSlotBlocked(PIItem const iitem) const;

	TIItemContainer			m_all;
	TIItemContainer			m_ruck;
	TIItemContainer			m_activ_last_items;
	TISlotArr				m_slots;
	TIICArr					m_pockets;

	TIItemContainer			m_to_check;
	xr_vector<CArtefact*>	m_artefacts;

public:
	//возвращает все кроме PDA в слоте и болта
	void					AddAvailableItems		(TIItemContainer& items_container, bool for_trade, bool bOverride = false) const;

	inline	CInventoryOwner*GetOwner				() const				{ return m_pOwner; }
	

	friend class CInventoryOwner;


	u32					ModifyFrame					() const					{ return m_dwModifyFrame; }
	void				InvalidateState				()							{ m_dwModifyFrame = Device.dwFrame; }
	bool				isBeautifulForActiveSlot	(CInventoryItem *pIItem);

	u16					m_iReturnPlace;
	u16 				m_iReturnSlot;
	u16					m_iNextActiveItemID;
	u16					m_iNextLeftItemID;
	
	u16					m_iToDropID;
	u16					m_iRuckBlockID;

protected:
	void				UpdateDropTasks				();
	void				UpdateDropItem				(PIItem pIItem);

	// јктивный слот и слот который станет активным после смены
    //значени€ совпадают в обычном состо€нии (нет смены слотов)
	u16 				m_iActiveSlot;
	u16 				m_iNextActiveSlot;
	u16 				m_iPrevActiveSlot;

	CInventoryOwner*	m_pOwner;
	bool				m_bActors;

	//флаг, допускающий использование слотов
	bool				m_bSlotsUseful;
	bool				m_bBoltPickUp;

	// текущий вес в инвентаре
	float				m_fTotalWeight;
	float				m_fTotalVolume;

	//кадр на котором произошло последнее изменение в инвенторе
	u32					m_dwModifyFrame;

	bool				m_drop_last_frame;

	bool				m_change_after_deactivate;

	void				SendActionEvent		(u16 cmd, u32 flags);

private:
	priority_group*		m_slot2_priorities[qs_priorities_count];
	priority_group*		m_slot3_priorities[qs_priorities_count];

	priority_group			m_groups[qs_priorities_count];
	priority_group			m_null_priority;
	typedef xr_set<PIItem>	except_next_items_t;
	except_next_items_t		m_next_items_exceptions;
	u32						m_next_item_iteration_time;

	std::vector<u8> m_blocked_slots;
	bool				IsSlotBlocked(u16 slot_id) const;
	void				TryActivatePrevSlot		();
	void				TryDeactivateActiveSlot	();
													
			void			OnInventoryAction		(PIItem item, u16 actionType = GE_OWNERSHIP_TAKE, u8 zone = 1);

private:
			void			CheckArtefact			(PIItem item, bool add = false);
};