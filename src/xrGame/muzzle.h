#pragma once
#include "module.h"

class CMuzzle : public CModule
{
public:
										CMuzzle									(CGameObject* obj, shared_str CR$ section);

private:
	const Fvector						m_muzzle_point;
	const Fvector						m_recoil_pattern;
	const bool							m_flash_hider;

public:
	Fvector CR$							getRecoilPattern					C$	()		{ return m_recoil_pattern; }
	bool								isFlashHider						C$	()		{ return m_flash_hider; }
	Fvector 							getFirePoint						C$	();
};
