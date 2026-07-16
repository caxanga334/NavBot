#include NAVBOT_PCH_FILE
#include <mods/zps/zps_lib.h>
#include <mods/zps/zps_mod.h>
#include <bot/zps/zpsbot.h>
#include <bot/tasks_shared/bot_shared_roam.h>
#include <bot/tasks_shared/bot_shared_squad_member_monitor.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include <bot/tasks_shared/bot_shared_use_entity.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include "objective/zpsbot_human_objectives_task.h"
#include "zpsbot_objective_human_task.h"

TaskResult<CZPSBot> CZPSBotObjectiveHumanTask::OnTaskStart(CZPSBot* bot, AITask<CZPSBot>* pastTask)
{
	return Continue();
}

TaskResult<CZPSBot> CZPSBotObjectiveHumanTask::OnTaskUpdate(CZPSBot* bot)
{
	TaskResult<CZPSBot> objresult = GetObjectiveTask(bot);

	if (objresult.IsRequestingChange())
	{
		return objresult;
	}

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

TaskEventResponseResult<CZPSBot> CZPSBotObjectiveHumanTask::OnCustomModEvent(CZPSBot* bot, const int id, const std::any& data)
{
	if (id != CZombiePanicSourceMod::MODEVENT_ZPS_NEW_OBJECTIVE)
	{
		return TryContinue(PRIORITY_LOW);
	}

	// restart this task to see what the bot can do about the new objective.
	// this will also stop any objective tasks the bot was doing.
	return TrySwitchTo(new CZPSBotObjectiveHumanTask, PRIORITY_MEDIUM, "Objective updated, restarting objective monitor!");
}

TaskResult<CZPSBot> CZPSBotObjectiveHumanTask::GetObjectiveTask(CZPSBot* bot) const
{
	CZombiePanicSourceMod* mod = CZombiePanicSourceMod::GetZPSMod();
	const CZPSObjectiveManager& mgr = mod->GetObjectiveManager();
	CZPSObjectiveManager::ObjectiveTypes objective = mgr.GetCurrentObjective();

	switch (objective)
	{
	case CZPSObjectiveManager::ObjectiveTypes::OBJECTIVE_MOVETO:
	{
		return PauseFor(new CBotSharedGoToPositionTask<CZPSBot, CZPSBotPathCost>(bot, mgr.GetMoveGoal(), "ObjectiveMoveTo"), "Completing MoveTo objective!");
	}
	case CZPSObjectiveManager::ObjectiveTypes::OBJECTIVE_USE_BUTTON:
	{
		CBaseEntity* entity = mgr.GetUseButton();

		if (entity)
		{
			return PauseFor(new CBotSharedUseEntityTask<CZPSBot, CZPSBotPathCost>(entity), "Completing UseButton objective!");
		}

		break;
	}
	case CZPSObjectiveManager::ObjectiveTypes::OBJECTIVE_FIND_ITEM:
	{
		return PauseFor(new CZPSBotObjectiveFindItemTask, "Finding an item!");
	}
	case CZPSObjectiveManager::ObjectiveTypes::OBJECTIVE_USE_ITEM:
	{
		if (CZPSBotObjectiveUseItemTask::IsPossible(bot))
		{
			return PauseFor(new CZPSBotObjectiveUseItemTask, "Using an item!");
		}

		// go find one
		return PauseFor(new CZPSBotObjectiveFindItemTask, "Finding an item!");
	}
	default:
		break;
	}


	return Continue();
}
