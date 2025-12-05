#include "stdafx.h"
#include "ActorBackpack.h"

#include "item_container.h"

bool CBackpack::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result{ _super::install_upgrade_impl(section, test) };
	if (auto cont = getModule<MContainer>())
	{
		float tmp{ 0.F };
		if (process_if_exists(section, "capacity", tmp, test))
		{
			cont->SetCapacity(cont->GetCapacity() + tmp);
			result = true;
		}
	}
	return result;
}
