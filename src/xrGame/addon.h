#pragma once
#include "attachment.h"
#include "addon_owner.h"

class CAddonOwner;

class CAddon
{
public:
	void									V$	Load									(LPCSTR section);

private:
	shared_str									m_section;

public:
	shared_str CR$								Section								C$	()		{ return m_section; }
};

class CAddonObject : public CAttachmentObject,
	public CAddon,
	public CAddonOwner
{
private:
	typedef	CAttachmentObject inherited;

public:
	DLL_Pure*									_construct							O$	() ;
	void										Load								O$	(LPCSTR section) ;

private:
	shared_str									m_SlotType;

public:
	shared_str CR$								SlotType							C$	()		{ return m_SlotType; }

	float										GetControlInertionFactor			CO$	();

	void										renderable_Render					O$	();
	void										UpdateAddonsTransform				O$	();
    void										render_hud_mode						O$	();
};
