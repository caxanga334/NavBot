#include NAVBOT_PCH_FILE
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include "cssbot_guard_rescue_zone_task.h"

AITask<CCSSBot>* CCSSBotGuardRescueZoneTask::InitialNextTask(CCSSBot* bot)
{
	CBaseEntity* rescuezone = UtilHelpers::GetRandomEntityOfClassname("func_hostage_rescue");

	if (rescuezone)
	{
		const Vector& pos = UtilHelpers::getWorldSpaceCenter(rescuezone);
		return new CBotSharedDefendSpotTask<CCSSBot, CCSSBotPathCost>(bot, pos, -1.0f, true);
	}

	return nullptr;
}

TaskResult<CCSSBot> CCSSBotGuardRescueZoneTask::OnTaskUpdate(CCSSBot* bot)
{
	if (GetNextTask() == nullptr)
	{
		return Done("Done camping!");
	}

	return Continue();
}
