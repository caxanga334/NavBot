#ifndef NAVBOT_BOT_INTERFACE_PATH_NATIVES_H_
#define NAVBOT_BOT_INTERFACE_PATH_NATIVES_H_

#include <vector>
#include <IExtensionSys.h>
#include <ISourceMod.h>

namespace natives::bots::interfaces::path
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	cell_t UpdateNavigator(IPluginContext* context, const cell_t* params);
	cell_t IsValid(IPluginContext* context, const cell_t* params);
	cell_t ComputeToPos(IPluginContext* context, const cell_t* params);
	cell_t GetMoveGoal(IPluginContext* context, const cell_t* params);
}

#endif // !NAVBOT_BOT_INTERFACE_PATH_NATIVES_H_
