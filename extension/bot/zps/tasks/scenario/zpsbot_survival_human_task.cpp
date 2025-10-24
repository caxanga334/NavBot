#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include "zpsbot_survival_human_task.h"

TaskResult<CZPSBot> CZPSBotSurvivalHumanTask::OnTaskUpdate(CZPSBot* bot)
{
    // Temporary
    return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 4096.0f), "Roaming!");
}
