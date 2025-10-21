#include NAVBOT_PCH_FILE
#include <string_view>
#include <extension.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dodslib.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include "dodsbot_tactical_monitor_task.h"
#include "dodsbot_main_task.h"

AITask<CDoDSBot>* CDoDSBotMainTask::InitialNextTask(CDoDSBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CDoDSBotTacticalMonitorTask;
}

TaskResult<CDoDSBot> CDoDSBotMainTask::OnTaskUpdate(CDoDSBot* bot)
{
	if (dodslib::GetRoundState() == dayofdefeatsource::DoDRoundState::DODROUNDSTATE_PREROUND)
	{
		bot->GetMovementInterface()->ClearStuckStatus("PREROUND");
		return Continue();
	}

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotMainTask::OnDebugMoveToCommand(CDoDSBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CDoDSBot, CDoDSBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

TaskEventResponseResult<CDoDSBot> CDoDSBotMainTask::OnKilled(CDoDSBot* bot, const CTakeDamageInfo& info)
{
	botsharedutils::SpreadDangerToNearbyAreas spread{ bot->GetLastKnownNavArea(), static_cast<int>(bot->GetMyDoDTeam()), 600.0f, CNavArea::ADD_DANGER_KILLED, CNavArea::MAX_DANGER_ONKILLED };
	spread.Execute();

	return TrySwitchTo(new CBotSharedDeadTask<CDoDSBot, CDoDSBotMainTask>, PRIORITY_MANDATORY, "I am dead!");
}

Vector CDoDSBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aimhelper.SelectAimPosition(static_cast<CDoDSBot*>(me), entity, desiredAim);
}

const CKnownEntity* CDoDSBotMainTask::SelectTargetThreat(CBaseBot* baseBot, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(baseBot, threat1, threat2);
}
