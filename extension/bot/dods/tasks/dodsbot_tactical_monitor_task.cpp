#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include "dodsbot_scenario_monitor_task.h"
#include "dodsbot_tactical_monitor_task.h"

AITask<CDoDSBot>* CDoDSBotTacticalMonitorTask::InitialNextTask(CDoDSBot* bot)
{
	return new CDoDSBotScenarioMonitorTask;
}

TaskResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnTaskUpdate(CDoDSBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnNavAreaChanged(CDoDSBot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	if (newArea && newArea->HasPrerequisite())
	{
		const CNavPrerequisite* prereq = newArea->GetPrerequisite();

		if (prereq->IsEnabled() && prereq != bot->GetLastUsedPrerequisite())
		{
			CNavPrerequisite::PrerequisiteTask task = prereq->GetTask();

			switch (task)
			{
			case CNavPrerequisite::TASK_WAIT:
				return TryPauseFor(new CBotSharedPrereqWaitTask<CDoDSBot>(prereq->GetFloatData()), PRIORITY_HIGH, "Prerequisite tells me to wait!");
			case CNavPrerequisite::TASK_MOVE_TO_POS:
				return TryPauseFor(new CBotSharedPrereqMoveToPositionTask<CDoDSBot, CDoDSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to move to a position!");
			case CNavPrerequisite::TASK_DESTROY_ENT:
				return TryPauseFor(new CBotSharedPrereqDestroyEntityTask<CDoDSBot, CDoDSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to destroy an entity!");
			case CNavPrerequisite::TASK_USE_ENT:
				return TryPauseFor(new CBotSharedPrereqUseEntityTask<CDoDSBot, CDoDSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to use an entity!");
			default:
				break;
			}
		}
	}

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnInjured(CDoDSBot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* attacker = info.GetAttacker();

		if (attacker)
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(attacker);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 1.5f, "Looking at attacker!");

			if (UtilHelpers::IsPlayer(attacker))
			{
				CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(attacker);
				known->UpdatePosition();
			}
		}
	}

	return TryContinue(PRIORITY_LOW);
}
