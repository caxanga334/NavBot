#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include "insmicbot_tactical_task.h"
#include "insmicbot_main_task.h"

AITask<CInsMICBot>* CInsMICBotMainTask::InitialNextTask(CInsMICBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CInsMICBotTacticalTask;
}

TaskResult<CInsMICBot> CInsMICBotMainTask::OnTaskUpdate(CInsMICBot* bot)
{
	return Continue();
}

const CKnownEntity* CInsMICBotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(me, threat1, threat2);
}

Vector CInsMICBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aimhelper.SelectAimPosition(static_cast<CInsMICBot*>(me), entity, desiredAim);
}

TaskEventResponseResult<CInsMICBot> CInsMICBotMainTask::OnDebugMoveToCommand(CInsMICBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CInsMICBot, CInsMICBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

TaskEventResponseResult<CInsMICBot> CInsMICBotMainTask::OnKilled(CInsMICBot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CBotSharedDeadTask<CInsMICBot, CInsMICBotMainTask>(), PRIORITY_MANDATORY, "I am dead!");
}
