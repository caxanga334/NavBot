#include NAVBOT_PCH_FILE
#include <extension.h>
#include "sdk_ehandle.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

IHandleEntity* CBaseHandle::Get() const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("[NavBot] CBaseHandle::Get()", "NavBot");
#endif // EXT_VPROF_ENABLED

	return g_EntList->LookupEntity(*this);
}


