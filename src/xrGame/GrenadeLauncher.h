#pragma once
#include "xmod\addon.h"
#include "xmod\attachment.h"

class CGrenadeLauncher : public CAddon
{
private:
	typedef CAddon inherited;

public:
												CGrenadeLauncher						();

		void									Load									(LPCSTR section) O$;

private:
		float m_fGrenadeVel;

public:
		float									GetGrenadeVel							() C$ { return m_fGrenadeVel; }
};

class CGrenadeLauncherObject : public CAttachmentObject,
	public CGrenadeLauncher
{
private:
	typedef	CAttachmentObject inherited;

public:
	void										Load									(LPCSTR section) O$;
};
