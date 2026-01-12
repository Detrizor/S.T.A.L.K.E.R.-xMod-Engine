#pragma once

class MAmountable : public CModule
{
public:
	static EModuleTypes mid() { return mAmountable; }

	using CallbackType = _STD function<void()>;

public:
	explicit MAmountable(CGameObject* obj);

private:
	void	sSyncData		(CSE_ALifeDynamicObject* se_obj, bool save) override;
	float	sSumItemData	(EItemDataTypes type) override;

	xoptional<float> sGetAmount	() override		{ return m_fAmount; }
	xoptional<float> sGetFill	() override		{ return get_fill(); }
	xoptional<float> sGetBar	() override		{ return Full() ? -1.F : get_fill(); }

private:
	float get_fill() const { return m_fAmount / m_fMaxAmount; }

	bool is_useful() const;

	void on_amount_change();

public:
	static float getBaseAmount(const shared_str& section);

	float getAmount			() const	{ return m_fAmount; }
	float getMaxAmount		() const	{ return m_fMaxAmount; }
	float getDepletionSpeed	() const	{ return m_fDepletionSpeed; }
	float getDepletionRate	() const	{ return m_fDepletionSpeed / m_fMaxAmount; }

	bool Empty	() const { return fIsZero(m_fAmount); }
	bool Full	() const { return fEqual(m_fAmount, m_fMaxAmount) && !m_bUnlimited; }

	void setDepletionSpeed	(float val)		{ m_fDepletionSpeed = val; }
	void deplete			()				{ changeAmount(-m_fDepletionSpeed); }

	void addOnAmountChangeCallback(CallbackType&& callback) { _onAmountChangeCallbacks.push_back(_STD move(callback)); }

	void setAmount		(float val);
	void changeAmount	(float delta);
	void setFill		(float val);
	void changeFill		(float delta);

private:
	xr_vector<CallbackType> _onAmountChangeCallbacks{};

	const bool m_bUnlimited;
	const bool m_bNetCostAdditive;

	const float m_fMaxAmount;
	const float m_fNetWeight;
	const float m_fNetVolume;
	const float m_fAmountCost;

	float m_fNetCost;
	float m_fDepletionSpeed;

	float m_fAmount{ 0.F };
};
