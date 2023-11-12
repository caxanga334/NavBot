#include <ifaces_extern.h>
#include "sdk_timers.h"

// work-around since client header doesn't like inlined gpGlobals->curtime
float IntervalTimer::Now(void) const
{
	return gpGlobals->curtime;
}

// work-around since client header doesn't like inlined gpGlobals->curtime
float CountdownTimer::Now(void) const
{
	return gpGlobals->curtime;
}