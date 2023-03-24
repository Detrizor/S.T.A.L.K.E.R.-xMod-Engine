#pragma once
#include "addon.h"

class CSilencer
{
public:
							CSilencer				()										{}
							~CSilencer				()										{}
	virtual	void			Load					(LPCSTR section);
};

class CSilencerObject : public CAddonObject,
	public CSilencer
{
private:
	typedef	CAddonObject	inherited;

public:
							CSilencerObject			()										{}
	virtual					~CSilencerObject		()										{}
	virtual	void			Load					(LPCSTR section);
};
