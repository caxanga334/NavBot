#ifndef __NAVBOT_NATIVES_H_
#define __NAVBOT_NATIVES_H_

#include <vector>
#include <IExtensionSys.h>
#include <ISourceMod.h>

#include "natives/bots.h"
#include "natives/navmesh.h"
#include "natives/interfaces/path.h"

namespace natives
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	cell_t IsNavBot(IPluginContext* context, const cell_t* params);
	cell_t FireNavBotSoundEvent(IPluginContext* context, const cell_t* params);
	cell_t GetNavBotCount(IPluginContext* context, const cell_t* params);
	cell_t BuildPathSimple(IPluginContext* context, const cell_t* params);
	cell_t GetPathSegment(IPluginContext* context, const cell_t* params);
	cell_t GetPathSegmentCount(IPluginContext* context, const cell_t* params);
}

#endif // !__NAVBOT_NATIVES_H_
