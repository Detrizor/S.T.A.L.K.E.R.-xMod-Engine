#include "stdafx.h"
#include "module.h"
#include "GameObject.h"

void CModule::Transfer(u16 id) const
{
	O.transfer(id);
}
