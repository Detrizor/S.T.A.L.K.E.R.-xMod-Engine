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

protected:
	float								_GetAmount							C$	()		{ return m_fAmount; }
	float								_GetFill							C$	()		{ return _GetAmount() / Capacity(); }
	float								_GetBar								C$	()		{ return Full() ? -1.f : _GetFill(); }

	float								_Weight								C$	()		{ return m_net_weight * _GetFill(); }
	float								_Volume								C$	()		{ return m_net_volume * _GetFill(); }
	float								_Cost								C$	()		{ return round(m_net_cost * (_GetFill() - 1.f)); }
};

class CIIOAmountable : public CInventoryItemObject,
	public CItemAmountable
{
private:
	typedef	CInventoryItemObject					inherited;

public:
	virtual DLL_Pure*		_construct				();

	virtual	void			Load					(LPCSTR section);

public:
	float								_Weight								CO$	()		{ return CItemAmountable::_Weight(); }
	float								_Volume								CO$	()		{ return CItemAmountable::_Volume(); }
	float								_Cost								CO$	()		{ return CItemAmountable::_Cost(); }

	float								_GetAmount							CO$	()		{ return CItemAmountable::_GetAmount(); }
	float								_GetFill							CO$	()		{ return CItemAmountable::_GetFill(); }
	float								_GetBar								CO$	()		{ return CItemAmountable::_GetBar(); }
};
