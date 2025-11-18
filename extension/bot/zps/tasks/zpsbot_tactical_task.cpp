#include NAVBOT_PCH_FILE
#include <bot/zps/zpsbot.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/tasks_shared/bot_shared_pursue_and_destroy.h>
#include <bot/interfaces/behavior_utils.h>
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
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CZPSBot, CZPSBotPathCost);

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
