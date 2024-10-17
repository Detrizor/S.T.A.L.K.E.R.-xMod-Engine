#pragma once
#include "module.h"

class CModuleOwner : virtual public CSignalProcessor
{
	CGameObject* const					obj;

public:
										CModuleOwner							(CGameObject* obj_) : obj(obj_)		{};
										~CModuleOwner							()									{ xr_delete(m_modules); }

private:
	void								check_modules							()		{ if (!m_modules) m_modules = xr_new<xptr<CModule>, CModule::mModuleTypesEnd>(nullptr); }

protected:
	xptr<CModule>*						m_modules								= nullptr;

public:
	template <typename M>
	M*									getModule							C$	()
	{
		if (m_modules)
			if (auto& m = m_modules[M::mid()])
				return					smart_cast<M*>(m.get());
		return							nullptr;
	}
	
	template <typename M, typename... Args>
	void								addModule								(Args&&... args)	{ check_modules(); m_modules[M::mid()].construct<M>(obj, _STD forward<Args>(args)...); }
	template <typename M>
	void								registerModule							(M* p)				{ check_modules(); m_modules[M::mid()].capture(p); }
	template <typename M>
	void								unregisterModule						(M* p)				{ check_modules(); m_modules[M::mid()].release(); }

	virtual float						Aboba									(EEventTypes type, void* data = nullptr, int param = 0);
};
