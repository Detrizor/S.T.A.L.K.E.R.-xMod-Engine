////////////////////////////////////////////////////////////////////////////
//	Module 		: trade_factor_parameters.h
//	Created 	: 13.01.2006
//  Modified 	: 13.01.2006
//	Author		: Dmitriy Iassenev
//	Description : trade factor parameters class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "associative_vector.h"
#include "trade_factors.h"

class CTradeFactorParameters {
public:
	typedef associative_vector<shared_str,CTradeFactors>	FACTORS;
	typedef FACTORS::const_iterator							FI;
	typedef const FI										CFI;

private:
	FACTORS						m_factors;
	IC CFI						find					(const shared_str& section) const;

public:
	IC							CTradeFactorParameters	();
	IC	void					clear					();
	IC	void					enable					(const shared_str& section, const CTradeFactors& factors);
	IC	bool					enabled					(const shared_str& section) const;
	IC	const CTradeFactors&	factors					(const shared_str& section) const;
};

#include "trade_factor_parameters_inline.h"