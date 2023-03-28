#pragma once
#include "addon.h"
#include "attachment.h"

class CGrenadeLauncher : public CAddon
{
private:
	typedef CAddon inherited;

public:
												CGrenadeLauncher						();

		void									Load								O$	(LPCSTR section);

private:
		float m_fGrenadeVel;

public:
		float									GetGrenadeVel						C$	()		{ return m_fGrenadeVel; }
};

class CGrenadeLauncherObject : public CAttachmentObject,
	public CGrenadeLauncher
{
private:
	typedef	CAttachmentObject inherited;

public:
	void										Load								O$	(LPCSTR section);
};
