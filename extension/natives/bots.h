#ifndef NAVBOT_BOTS_NATIVES_H_
#define NAVBOT_BOTS_NATIVES_H_

#include <vector>
#include <IExtensionSys.h>
#include <ISourceMod.h>

namespace natives::bots
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	cell_t AddNavBotMM(IPluginContext* context, const cell_t* params);
	cell_t AttachNavBot(IPluginContext* context, const cell_t* params);
	cell_t IsPluginBot(IPluginContext* context, const cell_t* params);
	cell_t GetNavBotByIndex(IPluginContext* context, const cell_t* params);
	cell_t SetRunPlayerCommands(IPluginContext* context, const cell_t* params);
}

#endif // !NAVBOT_BOTS_NATIVES_H_
