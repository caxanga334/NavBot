#include NAVBOT_PCH_FILE
#include <extension.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/tasks_shared/bot_shared_take_cover_from_danger.h>
#include <bot/tasks_shared/bot_shared_plugin_command_tasks.h>
#include <bot/interfaces/behavior_utils.h>
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
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CDoDSBot, CDoDSBotPathCost);

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

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnDangerousEntityChanged(CDoDSBot* bot, CBaseEntity* newent, CBaseEntity* oldent)
{
	BOTVEHAVIOR_IMPLEMENT_SIMPLE_DANGER_COVER(CDoDSBot, CDoDSBotPathCost);

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnPluginCommand(CDoDSBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data)
{
	return plugincommandtask::ImplementPluginCommandTasks<CDoDSBotTacticalMonitorTask, CDoDSBot, CDoDSBotPathCost>(this, bot, type, data);
}
