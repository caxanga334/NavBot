#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include <bot/tasks_shared/bot_shared_investigate_heard_entity.h>
#include <bot/tasks_shared/bot_shared_investigate_known.h>
#include <bot/tasks_shared/bot_shared_patrol_uncleared_areas.h>
#include "zpsbot_survival_zombie_task.h"

TaskResult<CZPSBot> CZPSBotSurvivalZombieTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
    if (bot->IsCarrier())
    {
        CZPSBotSquad* squad = bot->GetSquadInterface();

        if (squad->CanLeadSquads() && CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
        {
            botsquadutils::TryFormSquadFunc func{ bot, nullptr, 3 };
            extmanager->ForEachBot(func);
            func.CreateSquad();
        }
    }
    else
    {
        if (bot->GetSquadInterface()->CanJoinSquads() && CBaseBot::s_botrng.GetRandomChance(bot->GetDifficultyProfile()->GetTeamwork()))
        {
            CBaseEntity* human = nullptr;
            CZPSBotSquad* squad = CZPSBotSquad::GetFirstCarrierSquadInterface(bot, &human);

            if (squad)
            {
                if (squad->IsInASquad())
                {
                    squad->AddMemberToSquad(bot);
                }
                else
                {
                    // carrier is a human, try forming a squad.
                    if (human)
                    {
                        botsquadutils::TryFormSquadFunc func{ bot, human, 3 };
                        extmanager->ForEachBot(func);
                        func.CreateSquad();
                    }
                }
            }
        }
    }

    return Continue();
}

TaskResult<CZPSBot> CZPSBotSurvivalZombieTask::OnTaskUpdate(CZPSBot* bot)
{
    const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

    // I see an enemy.
    if (threat)
    {
        return PauseFor(new CBotSharedDefaultCombatBehaviorTask<CZPSBot, CZPSBotPathCost>, "Attacking visible threat!");
    }

    CBaseEntity* target;
    // Investigate if possible.
    if (CBotSharedInvestigateKnownTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, &target))
    {
        return PauseFor(new CBotSharedInvestigateKnownTask<CZPSBot, CZPSBotPathCost>(target), "Investigating known enemy position!");
    }

    // Patrol chance based on game awareness skill, capped at 90%
    if (CBaseBot::s_botrng.GetRandomChance(std::min(bot->GetDifficultyProfile()->GetGameAwareness(), 90)))
    {
        CNavArea* area;
        if (CBotSharedPatrolUnclearedAreasTask<CZPSBot, CZPSBotPathCost>::IsPossible(bot, &area))
        {
            return PauseFor(new CBotSharedPatrolUnclearedAreasTask<CZPSBot, CZPSBotPathCost>(area), "Patrolling for enemies.");
        }
    }

    // Move randomly as a fallback
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
