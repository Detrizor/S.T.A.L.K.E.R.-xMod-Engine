#pragma once
#include "module.h"

class CModuleOwner : virtual public CSignalProcessor
{
	CGameObject* const					obj;

public:
										CModuleOwner							(CGameObject* obj_) : obj(obj_)		{};
										~CModuleOwner							()									{ xr_delete(m_modules); }

private:
	xptr<CModule>*						m_modules								= nullptr;

	void								check_modules							()		{ if (!m_modules) m_modules = xr_new<xptr<CModule>, CModule::mModuleTypesEnd>(nullptr); }

public:
	template <typename M, typename... Args>
	void								addModule(Args&&... args)
	{
		check_modules();
		m_modules[M::mid()].construct<M>(obj, _STD forward<Args>(args)...);
	}

	template <typename M>
	void								registerModule							(M* p)				{ check_modules(); m_modules[M::mid()].capture(p); }
	template <typename M>
	void								unregisterModule						(M* p)				{ check_modules(); m_modules[M::mid()].release(); }

	template <typename M>
	M*									getModule							C$	()
	{
		if (m_modules)
			if (auto& m = m_modules[M::mid()])
				return					smart_cast<M*>(m.get());
		return							nullptr;
	}
	
	template <typename T>
	void								emitSignal_(T&& functor)
	{
		if (m_modules)
			for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
				if (auto& m = m_modules[i])
					functor				(m.get());
		functor							(this);
	}

	template <typename T>
	auto								emitSignalSum_(T&& functor)
	{
		auto res						= functor(this);
		if (m_modules)
			for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
				if (auto& m = m_modules[i])
					res					+= functor(m.get());
		return							res;
	}

	template <typename T>
	bool								emitSignalDis_(T&& functor)
	{
		bool res						= functor(this);
		if (m_modules)
			for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
				if (auto& m = m_modules[i])
					res					|= functor(m.get());
		return							res;
	}

	template <typename T>
	bool								emitSignalCon_(T&& functor)
	{
		bool res						= functor(this);
		if (m_modules)
			for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
				if (auto& m = m_modules[i])
					res					&= functor(m.get());
		return							res;
	}

	template <typename T>
	auto								emitSignalGet_(T&& functor)
	{
		if (m_modules)
			for (int i = CModule::mModuleTypesBegin; i < CModule::mModuleTypesEnd; ++i)
				if (auto& m = m_modules[i])
					if (auto res = functor(m.get()))
						return			res;
		return							functor(this);
	}
};

#define emitSignal(signal) emitSignal_([&](CSignalProcessor* p) { p->signal; })
#define emitSignalSum(signal) emitSignalSum_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalDis(signal) emitSignalDis_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalCon(signal) emitSignalCon_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalGet(signal) emitSignalGet_([&](CSignalProcessor* p) { return p->signal; })
