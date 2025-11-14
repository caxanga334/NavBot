#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include "zpsbot_scenario_task.h"
#include "zpsbot_tactical_task.h"

AITask<CZPSBot>* CZPSBotTacticalTask::InitialNextTask(CZPSBot* bot)
{
	return new CZPSBotScenarioTask;
}

TaskResult<CZPSBot> CZPSBotTacticalTask::OnTaskUpdate(CZPSBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CZPSBot> CZPSBotTacticalTask::OnInjured(CZPSBot* bot, const CTakeDamageInfo& info)
{
	CBaseEntity* attacker = info.GetAttacker();

	if (attacker)
	{
		Vector center = UtilHelpers::getWorldSpaceCenter(attacker);
		bot->GetControlInterface()->AimAt(center, IPlayerController::LOOK_DANGER, 1.5f, "Looking at my attacker!");
	}

	return TryContinue();
}

TaskEventResponseResult<CZPSBot> CZPSBotTacticalTask::OnNavAreaChanged(CZPSBot* bot, CNavArea* oldArea, CNavArea* newArea)
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
				return TryPauseFor(new CBotSharedPrereqWaitTask<CZPSBot>(prereq->GetFloatData()), PRIORITY_HIGH, "Prerequisite tells me to wait!");
			case CNavPrerequisite::TASK_MOVE_TO_POS:
				return TryPauseFor(new CBotSharedPrereqMoveToPositionTask<CZPSBot, CZPSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to move to a position!");
			case CNavPrerequisite::TASK_DESTROY_ENT:
				return TryPauseFor(new CBotSharedPrereqDestroyEntityTask<CZPSBot, CZPSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to destroy an entity!");
			case CNavPrerequisite::TASK_USE_ENT:
				return TryPauseFor(new CBotSharedPrereqUseEntityTask<CZPSBot, CZPSBotPathCost>(bot, prereq), PRIORITY_HIGH, "Prerequisite tells me to use an entity!");
			default:
				break;
			}
		}
	}

	return TryContinue();
}

TaskEventResponseResult<CZPSBot> CZPSBotTacticalTask::OnSound(CZPSBot* bot, CBaseEntity* source, const Vector& position, SoundType type, const float maxRadius)
{
	if (source)
	{
		if (!bot->GetSensorInterface()->IsIgnored(source) && bot->GetSensorInterface()->IsEnemy(source))
		{
			const float range = (bot->GetAbsOrigin() - position).Length();

			if (range <= maxRadius && range <= bot->GetSensorInterface()->GetMaxHearingRange())
			{
				bot->GetControlInterface()->AimAt(position, IPlayerController::LOOK_ALERT, 1.0f, "Looking at a noise I heard.");
			}
		}
	}

	return TryContinue();
}
