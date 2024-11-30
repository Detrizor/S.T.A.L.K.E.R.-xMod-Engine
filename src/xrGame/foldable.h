#pragma once
#include "module.h"

class MFoldable : public CModule
{
public:
	static EModuleTypes					mid										()		{ return mFoldable; }

public:
										MFoldable								(CGameObject* obj) : CModule(obj) {}

private:
	bool								m_status								= false;
	
	void								on_status_change					C$	(bool new_status);
	
	void								sSyncData							O$	(CSE_ALifeDynamicObject* se_obj, bool save);

public:
	void								setStatus								(bool status);
	
	bool								getStatus							C$	()		{ return m_status; }
};
