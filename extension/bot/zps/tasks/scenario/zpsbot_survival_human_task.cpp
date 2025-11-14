#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_hide.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include "zpsbot_survival_human_task.h"

TaskResult<CZPSBot> CZPSBotSurvivalHumanTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
    if (CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
    {
        if (!bot->GetSquadInterface()->IsInASquad())
        {
            bot->GetSquadInterface()->CreateSquad(nullptr);

            ISquad::InviteBotsToSquadFunc func{ bot, 3 };
            extmanager->ForEachBot(func);
        }
    }

    if (bot->GetSquadInterface()->IsInASquad() && !bot->GetSquadInterface()->IsSquadLeader())
    {
        return SwitchTo(new CBotSharedSquadMemberMonitorTask<CZPSBot, CZPSBotPathCost>(new CZPSBotSurvivalHumanTask), "Starting squad behavior!");
    }

    return Continue();
}

TaskResult<CZPSBot> CZPSBotSurvivalHumanTask::OnTaskUpdate(CZPSBot* bot)
{
    if (CBaseBot::s_botrng.GetRandomInt<int>(1, 7) == 7)
    {
        const HidingSpot* spot = nullptr;
        if (CBotSharedHideTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, 4096.0f, &spot))
        {
            return PauseFor(new CBotSharedHideTask<CZPSBot, CZPSBotPathCost>(bot, spot, CBaseBot::s_botrng.GetRandomReal<float>(10.0f, 20.0f)), "Hiding!");
        }
    }

    // Temporary
    return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 4096.0f), "Roaming!");
}

TaskEventResponseResult<CZPSBot> CZPSBotSurvivalHumanTask::OnSquadEvent(CZPSBot* bot, SquadEventType evtype)
{
    switch (evtype)
    {
    case SquadEventType::SQUAD_EVENT_JOINED:
    {
        return TrySwitchTo(new CBotSharedSquadMemberMonitorTask<CZPSBot, CZPSBotPathCost>(new CZPSBotSurvivalHumanTask), PRIORITY_HIGH, "I have joined a squad!");
    }
    default:
        break;
    }

    return TryContinue();
}
