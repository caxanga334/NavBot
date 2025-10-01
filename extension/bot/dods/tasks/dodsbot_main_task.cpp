#include NAVBOT_PCH_FILE
#include <string_view>
#include <extension.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dodslib.h>
#include <bot/bot_shared_utils.h>
#include "dodsbot_tactical_monitor_task.h"
#include "dodsbot_main_task.h"
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>

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

Vector CDoDSBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return botsharedutils::aiming::DefaultBotAim(me, entity, desiredAim);
}

const CKnownEntity* CDoDSBotMainTask::SelectTargetThreat(CBaseBot* baseBot, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	CDoDSBot* me = static_cast<CDoDSBot*>(baseBot);

	// check for cases where one of them is NULL

	if (threat1 && !threat2)
	{
		return threat1;
	}
	else if (!threat1 && threat2)
	{
		return threat2;
	}
	else if (threat1 == threat2)
	{
		return threat1; // if both are the same, return threat1
	}

	// both are valids now

	float range1 = me->GetRangeToSqr(threat1->GetLastKnownPosition());
	float range2 = me->GetRangeToSqr(threat2->GetLastKnownPosition());

	if (range1 < range2)
	{
		return threat1;
	}

	return threat2;
}
