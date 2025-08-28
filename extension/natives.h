#ifndef NAVBOT_NATIVES_H_
#define NAVBOT_NATIVES_H_

#include <vector>
#include <IExtensionSys.h>
#include <ISourceMod.h>

#include "natives/bots.h"
#include "natives/interfaces/path.h"

namespace natives
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	cell_t IsNavBot(IPluginContext* context, const cell_t* params);
	cell_t AddNavBot(IPluginContext* context, const cell_t* params);
	cell_t IsNavMeshLoaded(IPluginContext* context, const cell_t* params);
	cell_t FireNavBotSoundEvent(IPluginContext* context, const cell_t* params);
	cell_t GetNavBotCount(IPluginContext* context, const cell_t* params);
}




#endif // !NAVBOT_NATIVES_H_
