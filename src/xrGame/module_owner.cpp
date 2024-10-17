#include "stdafx.h"
#include "module_owner.h"
#include "GameObject.h"

float CModuleOwner::Aboba(EEventTypes type, void* data, int param)
{
	float res							= flt_max;
	if (m_modules)
	{
		for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
		{
			if (auto& m = m_modules[i])
			{
				float mres				= m->aboba(type, data, param);
				if (mres != flt_max)
				{
					if (res == flt_max)
						res				= 0.f;
					res					+= mres;
				}
			}
		}
	}
	return								res;
}
