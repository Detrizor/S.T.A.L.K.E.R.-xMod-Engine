////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_parameters_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeParameters::CTradeParameters						(const shared_str& section) :
	m_buy	(
		CTradeFactors(
			pSettings->r_float(section,"buy_price_factor_hostile"),
			pSettings->r_float(section,"buy_price_factor_friendly")
		)
	),
	m_sell	(
		CTradeFactors(
			pSettings->r_float(section,"sell_price_factor_hostile"),
			pSettings->r_float(section,"sell_price_factor_friendly")
		)
	)
{
}

IC	void CTradeParameters::clear							()
{
	m_buy.clear				();
	m_sell.clear			();
}

IC	CTradeParameters& CTradeParameters::instance			()
{
	if (m_instance)
		return				(*m_instance);

	m_instance				= xr_new<CTradeParameters>();
	return					(*m_instance);
}

IC	void CTradeParameters::clean							()
{
	xr_delete				(m_instance);
}

IC	CTradeParameters& default_trade_parameters				()
{
	return					(CTradeParameters::instance());
}

IC	const CTradeActionParameters& CTradeParameters::action	(action_buy) const
{
	return					(m_buy);
}

IC	const CTradeActionParameters& CTradeParameters::action	(action_sell) const
{
	return					(m_sell);
}

IC	const CTradeBoolParameters& CTradeParameters::action	(action_show) const
{
	return					(m_show);
}

IC	CTradeActionParameters& CTradeParameters::action		(action_buy)
{
	return					(m_buy);
}

IC	CTradeActionParameters& CTradeParameters::action		(action_sell)
{
	return					(m_sell);
}

IC	CTradeBoolParameters& CTradeParameters::action			(action_show)
{
	return					(m_show);
}

template <typename _action_type>
IC	bool CTradeParameters::enabled							(_action_type type, const shared_str& section) const
{
	return					action(type).enabled(section);
}

template <typename _action_type>
IC	const CTradeFactors& CTradeParameters::factors			(_action_type type, const shared_str& section) const
{
	VERIFY					(enabled(type, section));
	return					action(type).factors(section);
}

template <typename _action_type>
IC	void CTradeParameters::process							(_action_type type, CInifile& ini_file, const shared_str& section)
{
	R_ASSERT2				(ini_file.section_exist(section), make_string("cannot find section %s", *section));

	CTradeActionParameters&	_action = action(type);
	_action.clear			();

	CInifile::Sect&			S = ini_file.r_section(section);
	for (CInifile::SectCIt I = S.Data.begin(); I != S.Data.end(); ++I)
	{
		if (I->second.size())
		{
			string256		temp0, temp1;
			LPCSTR			param1 = _GetItem(*I->second, 0, temp0);
			LPCSTR			param2 = (_GetItemCount(*I->second) >= 2) ? _GetItem(*I->second, 1, temp1) : param1;
			_action.enable	(I->first, CTradeFactors((float)atof(param1), (float)atof(param2)));
		}
		else
			_action.enable	(I->first, CTradeFactors(-1.f, -1.f));
	}
}

template <typename _action_type>
IC	void CTradeParameters::default_factors					(_action_type type, const CTradeFactors& trade_factors)
{
	action(type).default_factors(trade_factors);
}