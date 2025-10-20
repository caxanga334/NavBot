#include NAVBOT_PCH_FILE
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include <bot/zps/zpsbot.h>
#include "zpsbot_main_task.h"

AITask<CZPSBot>* CZPSBotMainTask::InitialNextTask(CZPSBot* bot)
{
	return nullptr;
}

TaskResult<CZPSBot> CZPSBotMainTask::OnTaskUpdate(CZPSBot* bot)
{
	return Continue();
}

const CKnownEntity* CZPSBotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(me, threat1, threat2);
}

Vector CZPSBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return botsharedutils::aiming::DefaultBotAim(me, entity, desiredAim);
}

TaskEventResponseResult<CZPSBot> CZPSBotMainTask::OnDebugMoveToCommand(CZPSBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CZPSBot, CZPSBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Debug command!");
}
