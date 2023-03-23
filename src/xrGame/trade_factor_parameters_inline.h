////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_factor_parameters_inline.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade factor parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CTradeFactorParameters::CTradeFactorParameters				()
{
}

IC	void CTradeFactorParameters::clear							()
{
	m_factors.clear			();
}

IC	void CTradeFactorParameters::enable							(const shared_str& section, const CTradeFactors& factors)
{
	CFI						I = m_factors.find(section);
	VERIFY					(I == m_factors.end());
	m_factors.insert		(std::make_pair(section, factors));
}

IC	bool CTradeFactorParameters::enabled						(const shared_str& section) const
{
	CFI						I = find(section);
	return					((I != m_factors.end()) && (I->second.friend_factor() != -1.f));
}

IC	const CTradeFactors& CTradeFactorParameters::factors		(const shared_str& section) const
{
	CFI						I = find(section);
	VERIFY					(I != m_factors.end());
	return					(I->second);
}

IC	CTradeFactorParameters::CFI CTradeFactorParameters::find	(const shared_str& section) const
{
	CFI						E = m_factors.end();
	FI						I = m_factors.find(section);
	if (I != E)
		return				I;

	LPCSTR					main_class = READ_IF_EXISTS(pSettings, r_string, section, "main_class", false);
	if (!main_class)
		return				I;
	LPCSTR					subclass = READ_IF_EXISTS(pSettings, r_string, section, "subclass", "nil");
	LPCSTR					division = READ_IF_EXISTS(pSettings, r_string, section, "division", "nil");
	string256				tmp0, tmp1;
	LPCSTR					class0 = strconcat(sizeof(tmp0), tmp0, main_class, ".", subclass);
	LPCSTR					class1 = strconcat(sizeof(tmp1), tmp1, class0, ".", division);

	I						= m_factors.find(class1);
	if (I != E)
		return				I;
	I						= m_factors.find(class0);
	if (I != E)
		return				I;
	I						= m_factors.find(main_class);
	return					I;
}