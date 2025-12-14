#pragma once

#include "module.h"

class CModuleOwner : public CSignalProcessor
{
	CGameObject* const m_pObj;

public:
	CModuleOwner(CGameObject* pObj) : m_pObj(pObj) {};

private:
	xarr<xptr<CModule>, CModule::mModuleTypesEnd> m_pModules{ nullptr };

public:
	template <typename M, typename... Args>
	void addModule(Args&&... args) { m_pModules[M::mid()].construct<M>(m_pObj, _STD forward<Args>(args)...); }

	template <typename M>
	M* getModule() const
	{
		if (auto& m{ m_pModules[M::mid()] })
			return static_cast<M*>(m.get());
		return nullptr;
	}
	
	template <typename T>
	void emitSignal_(T&& functor)
	{
		for (auto& m : m_pModules)
			if (m)
				functor(m.get());
		functor(this);
	}

	template <typename T>
	auto emitSignalSum_(T&& functor)
	{
		auto res{ functor(this) };
		for (auto& m : m_pModules)
			if (m)
				res += functor(m.get());
		return res;
	}

	template <typename T>
	bool emitSignalDis_(T&& functor)
	{
		bool res{ functor(this) };
		for (auto& m : m_pModules)
			if (m)
				res |= functor(m.get());
		return res;
	}

	template <typename T>
	bool emitSignalCon_(T&& functor)
	{
		bool res{ functor(this) };
		for (auto& m : m_pModules)
			if (m)
				res &= functor(m.get());
		return res;
	}

	template <typename T>
	auto emitSignalGet_(T&& functor)
	{
		for (auto& m : m_pModules)
			if (m)
				if (auto res{ functor(m.get()) })
					return res;
		return functor(this);
	}
};

#define emitSignal(signal) emitSignal_([&](CSignalProcessor* p) { p->signal; })
#define emitSignalSum(signal) emitSignalSum_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalDis(signal) emitSignalDis_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalCon(signal) emitSignalCon_([&](CSignalProcessor* p) { return p->signal; })
#define emitSignalGet(signal) emitSignalGet_([&](CSignalProcessor* p) { return p->signal; })
