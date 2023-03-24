#pragma once
#include "inventory_item_object.h"

class CItemAmountable
{
public:
							CItemAmountable			();
	virtual					~CItemAmountable		()								{};
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);

private:
			PIItem			m_item;
			CPhysicItem*	m_object;
			float			m_net_weight;
			float			m_net_volume;
			u32				m_net_cost;
			float			m_capacity;
			bool			m_unlimited;
			float			m_depletion_speed;

			float			m_fAmount;

			void			OnAmountChange			();
			bool			Useful					() const;

public:

			float			Capacity				() const						{ return m_capacity; }
			float			GetDepletionSpeed		() const						{ return m_depletion_speed; }
			float			GetDepletionRate		() const						{ return GetDepletionSpeed() / Capacity(); }
			void			SetDepletionSpeed		(float val)						{ m_depletion_speed = val; }
			void			Deplete					()								{ ChangeAmount(-m_depletion_speed); }
			bool			Empty					() const						{ return fIsZero(m_fAmount); }
			bool			Full					() const						{ return fEqual(m_fAmount, m_capacity); }

			void			SetAmount				(float val);
			void			ChangeAmount			(float delta);
			void			SetFill					(float val);
			void			ChangeFill				(float delta);
			float			NetWeight				() const;
			float			NetVolume				() const;
			u32				NetCost					() const;
	
	virtual	float			GetAmount				() const						{ return m_fAmount; }
	virtual	float			GetFill					() const						{ return GetAmount() / Capacity(); }
	virtual float			GetBar					() const						{ return Full() ? -1.f : GetFill(); }
};

class CIIOAmountable : public CInventoryItemObject,
	public CItemAmountable
{
private:
	typedef	CInventoryItemObject					inherited;

public:
							CIIOAmountable			()								{};
	virtual					~CIIOAmountable			()								{};
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);

public:
	virtual float			Weight					() const;
	virtual float			Volume					() const;
	virtual u32				Cost					() const;
	virtual	float			GetAmount				() const						{ return CItemAmountable::GetAmount(); }
	virtual	float			GetFill					() const						{ return CItemAmountable::GetFill(); }
	virtual float			GetBar					() const						{ return CItemAmountable::GetBar(); }
};