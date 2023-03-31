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
			float			m_net_cost;
			float			m_capacity;
			bool			m_unlimited;
			float			m_depletion_speed;
			
	float								m_fAmount;

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

	float								GetAmount							C$	()		{ return m_fAmount; }
	float								GetFill								C$	()		{ return GetAmount() / Capacity(); }
	float								GetBar								C$	()		{ return Full() ? -1.f : GetFill(); }

	float								Weight								C$	()		{ return m_net_weight * GetFill(); }
	float								Volume								C$	()		{ return m_net_volume * GetFill(); }
	float								Cost								C$	()		{ return round(m_net_cost * (GetFill() - 1.f)); }
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
	float								Weight								CO$	()		{ return CItemAmountable::Weight(); }
	float								Volume								CO$	()		{ return CItemAmountable::Volume(); }
	float								Cost								CO$	()		{ return CItemAmountable::Cost(); }

	float								GetAmount							CO$	()		{ return CItemAmountable::GetAmount(); }
	float								GetFill								CO$	()		{ return CItemAmountable::GetFill(); }
	float								GetBar								CO$	()		{ return CItemAmountable::GetBar(); }
};
