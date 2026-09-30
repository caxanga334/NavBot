#ifndef __NAVBOT_SOURCEPAWN_SPAPI_BRIDGE_H_
#define __NAVBOT_SOURCEPAWN_SPAPI_BRIDGE_H_

/*
* SourcePawn API bridge functions. Primarily to avoid issues with circular dependency.
*/

class CBaseBot;
class Vector;

namespace navbot::spapi::bridge
{
	void OnComputePathFailed(const CBaseBot* bot, const Vector& goal, const float adjustedZ, bool nullstart);
}

#endif // !__NAVBOT_SOURCEPAWN_SPAPI_BRIDGE_H_
