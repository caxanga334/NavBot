#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include <bot/tasks_shared/bot_shared_investigate_heard_entity.h>
#include "zpsbot_survival_zombie_task.h"

TaskResult<CZPSBot> CZPSBotSurvivalZombieTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
    if (bot->IsCarrier())
    {
        CZPSBotSquad* squad = bot->GetSquadInterface();

        if (squad->IsInASquad())
        {
            squad->LeaveSquad();
        }

        if (CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
        {
            if (squad->CreateSquad(nullptr))
            {
                ISquad::InviteBotsToSquadFunc func{ bot, 3 };
                extmanager->ForEachBot(func);
            }
        }
    }
    else
    {
        if (CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
        {
            CZPSBotSquad* squad = CZPSBotSquad::GetFirstCarrierSquadInterface(bot);

            if (squad)
            {
                squad->TryToAddToSquad(bot);
            }
        }
    }

    return Continue();
}

TaskResult<CZPSBot> CZPSBotSurvivalZombieTask::OnTaskUpdate(CZPSBot* bot)
{
    // Temporary
    return PauseFor(new CBotSharedRoamTask<CZPSBot, CZPSBotPathCost>(bot, 4096.0f, true, -1.0f, true), "Roaming!");
}

TaskEventResponseResult<CZPSBot> CZPSBotSurvivalZombieTask::OnSquadEvent(CZPSBot* bot, SquadEventType evtype)
{
    switch (evtype)
    {
    case SquadEventType::SQUAD_EVENT_JOINED:
    {
        return TrySwitchTo(new CBotSharedSquadMemberMonitorTask<CZPSBot, CZPSBotPathCost>(new CZPSBotSurvivalZombieTask), PRIORITY_HIGH, "I have joined a squad!");
    }
    default:
        break;
    }

    return TryContinue();
}
