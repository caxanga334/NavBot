#include NAVBOT_PCH_FILE
#include <mods/zps/zps_lib.h>
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include "zpsbot_objective_human_task.h"

TaskResult<CZPSBot> CZPSBotObjectiveHumanTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	return Continue();
}

TaskResult<CZPSBot> CZPSBotObjectiveHumanTask::OnTaskUpdate(CZPSBot* bot)
{
    // random human survivor
    CBaseEntity* survivor = zpslib::GetRandomLivingSurvivor(true);

    if (survivor)
    {
        if (CBaseBot::s_botrng.GetRandomChance(35))
        {
            return PauseFor(new CBotSharedEscortEntityTask<CZPSBot, CZPSBotPathCost>(bot, survivor), "Following a random human teammate!");
        }

        return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, UtilHelpers::getEntityOrigin(survivor)), "Roaming to a random human teammate!");
    }

    // fallback to roaming around
    return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 4096.0f), "Roaming!");
}

TaskEventResponseResult<CZPSBot> CZPSBotObjectiveHumanTask::OnSquadEvent(CZPSBot* bot, SquadEventType evtype)
{
    switch (evtype)
    {
    case SquadEventType::SQUAD_EVENT_JOINED:
    {
        return TrySwitchTo(new CBotSharedSquadMemberMonitorTask<CZPSBot, CZPSBotPathCost>(new CZPSBotObjectiveHumanTask), PRIORITY_HIGH, "I have joined a squad!");
    }
    default:
        break;
    }

    return TryContinue();
}
