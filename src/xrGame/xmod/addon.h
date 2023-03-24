#pragma once
#include "attachment.h"
#include "addon_owner.h"

class CAddonOwner;

class CAddon
{
public:
							CAddon					()										{}
							~CAddon					()										{}

	virtual	void			Load					(LPCSTR section);

private:
			shared_str		m_section;
			shared_str		m_SlotType;

public:
	const	shared_str&		Section					()								const	{ return m_section; }
	const	shared_str&		SlotType				()								const	{ return m_SlotType; }
};

class CAddonObject : public CAttachmentObject,
	public CAddon,
	public CAddonOwner
{
private:
	typedef	CAttachmentObject	inherited;

public:
							CAddonObject			()										{}
	virtual					~CAddonObject			()										{}
	virtual DLL_Pure*		_construct				();


	virtual	void			Load					(LPCSTR section);
	virtual	void			OnEvent					(NET_Packet& P, u16 type);
			
public:
    virtual void			renderable_Render		();
	virtual void			UpdateAddonsTransform	();
    virtual void			render_hud_mode			();
	virtual	float			GetControlInertionFactor()								const;
};
