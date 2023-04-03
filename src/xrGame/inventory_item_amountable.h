#pragma once
#include "inventory_item_object.h"

class CAmountable : public CModule
{
public:
										CAmountable								(CGameObject* obj);

private:
	float								m_net_weight;
	float								m_net_volume;
	float								m_net_cost;
	float								m_capacity;
	bool								m_unlimited;
	float								m_depletion_speed;
	PIItem								m_item;

	float								m_fAmount;

	float								Fill								C$	()		{ return m_fAmount / m_capacity; }

	void								OnAmountChange							();
	bool								Useful								C$	();
	float								aboba								O$	(EEventTypes type, void* data, int param);

public:
	float								Capacity								() const						{ return m_capacity; }
	float								GetDepletionSpeed						() const						{ return m_depletion_speed; }
	float								GetDepletionRate						() const						{ return GetDepletionSpeed() / Capacity(); }
	void								SetDepletionSpeed						(float val)						{ m_depletion_speed = val; }
	void								Deplete									()								{ ChangeAmount(-m_depletion_speed); }
	bool								Empty									() const						{ return fIsZero(m_fAmount); }
	bool								Full									() const						{ return fEqual(m_fAmount, m_capacity); }

	void								SetAmount								(float val);
	void								ChangeAmount							(float delta);
	void								SetFill									(float val);
	void								ChangeFill								(float delta);
};
