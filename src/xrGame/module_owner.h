#pragma once
#include "module.h"

class CModuleOwner : public CSignalProcessor
{
	CGameObject* const					obj;

public:
										CModuleOwner							(CGameObject* obj_) : obj(obj_)		{};
										~CModuleOwner							()									{ for (auto& m : m_modules) xr_delete(m); }

private:
	CModule*							m_modules[CModule::mModuleTypesEnd]		= {};

public:
	template <typename M, typename... Args>
	void								addModule								(Args&&... args) { m_modules[M::mid()] = xr_new<M>(obj, _STD forward<Args>(args)...); }

	template <typename M>
	M*									getModule							C$	()
	{
		if (auto& m = m_modules[M::mid()])
			return						static_cast<M*>(m);
		return							nullptr;
	}
	
	template <typename T>
	void								emitSignal_								(T&& functor)
	{
		for (auto& m : m_modules)
			if (m)
				functor					(m);
		functor							(this);
	}

	template <typename T>
	auto								emitSignalSum_							(T&& functor)
	{
		auto res						= functor(this);
		for (auto& m : m_modules)
			if (m)
				res						+= functor(m);
		return							res;
	}

	template <typename T>
	bool								emitSignalDis_							(T&& functor)
	{
		bool res						= functor(this);
		for (auto& m : m_modules)
			if (m)
				res						|= functor(m);
		return							res;
	}

	template <typename T>
	bool								emitSignalCon_							(T&& functor)
	{
		bool res						= functor(this);
		for (auto& m : m_modules)
			if (m)
				res						&= functor(m);
		return							res;
	}

	template <typename T>
	auto								emitSignalGet_							(T&& functor)
	{
		for (auto& m : m_modules)
			if (m)
				if (auto res = functor(m))
					return				res;
		return							functor(this);
	}
};

#define emitSignal(signal) emitSignal_([&](CSignalProcessor* p) { p->signal; })
#define emitSignalSum(signal) emitSignalSum_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalDis(signal) emitSignalDis_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalCon(signal) emitSignalCon_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalGet(signal) emitSignalGet_([&](CSignalProcessor* p) { return p->signal; })
