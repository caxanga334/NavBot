#include <extension.h>
#include "sdk_ehandle.h"

IHandleEntity* CBaseHandle::Get() const
{
	return g_EntList->LookupEntity(*this);
}


