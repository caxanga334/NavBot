#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "zpsbot_survival_zombie_task.h"

TaskResult<CZPSBot> CZPSBotSurvivalZombieTask::OnTaskUpdate(CZPSBot* bot)
{
    // Temporary
    return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 4096.0f, true, -1.0f, true), "Roaming!");
}
