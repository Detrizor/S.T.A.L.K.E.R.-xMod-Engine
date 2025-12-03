#pragma once

class MAmountable : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mAmountable; }

public:
										MAmountable								(CGameObject* obj);

private:
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);
	float								sSumItemData						O$	(EItemDataTypes type);
	xoptional<float>					sGetAmount							O$	()		{ return m_amount; }
	xoptional<float>					sGetFill							O$	()		{ return get_fill(); }
	xoptional<float>					sGetBar								O$	()		{ return Full() ? -1.f : get_fill(); }

private:
	float								m_net_weight;
	float								m_net_volume;
	float								m_net_cost;
	float								m_capacity;
	bool								m_unlimited;
	float								m_depletion_speed;

	float								m_amount								= 0.f;

	void								OnAmountChange							();
	void								get_base_amount							();

	float								get_fill							C$	()		{ return m_amount / m_capacity; }
	bool								Useful								C$	();

public:
	void								setCapacity								(float val)		{ m_capacity = val; }
	void								SetDepletionSpeed						(float val)		{ m_depletion_speed = val; }
	void								Deplete									()				{ ChangeAmount(-m_depletion_speed); }

	void								SetAmount								(float val);
	void								ChangeAmount							(float delta);
	void								SetFill									(float val);
	void								ChangeFill								(float delta);

	float								getAmount							C$	()		{ return m_amount; }
	float								getCapacity							C$	()		{ return m_capacity; }
	float								GetDepletionSpeed					C$	()		{ return m_depletion_speed; }
	float								GetDepletionRate					C$	()		{ return m_depletion_speed / m_capacity; }
	bool								Empty								C$	()		{ return fIsZero(m_amount); }
	bool								Full								C$	()		{ return fEqual(m_amount, m_capacity) && !m_unlimited; }
};
