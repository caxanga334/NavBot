#include NAVBOT_PCH_FILE
#include <manager.h>
#include <bot/basebot.h>
#include "spapi_bridge.h"

namespace navbot::spapi::bridge
{
	void OnComputePathFailed(const CBaseBot* bot, const Vector& goal, const float adjustedZ, bool nullstart)
	{
#ifndef NO_SOURCEPAWN_API
		extmanager->SMAPI_OnNavBotComputePathFailed(bot, goal, adjustedZ, nullstart);
#endif // !NO_SOURCEPAWN_API
	}
}


