#include NAVBOT_PCH_FILE
#include <bot/hl1mp/hl1mp_bot.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include "hl1mp_bot_tactical_task.h"
#include "hl1mp_bot_main_task.h"

AITask<CHL1MPBot>* CHL1MPBotMainTask::InitialNextTask(CHL1MPBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CHL1MPBotTacticalTask;
}

TaskResult<CHL1MPBot> CHL1MPBotMainTask::OnTaskUpdate(CHL1MPBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CHL1MPBot> CHL1MPBotMainTask::OnDebugMoveToCommand(CHL1MPBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CHL1MPBot, CHL1MPBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

TaskEventResponseResult<CHL1MPBot> CHL1MPBotMainTask::OnKilled(CHL1MPBot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CBotSharedDeadTask<CHL1MPBot, CHL1MPBotMainTask>(), PRIORITY_CRITICAL, "I am dead!");
}

Vector CHL1MPBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aimhelper.SelectAimPosition(static_cast<CHL1MPBot*>(me), entity, desiredAim);
}

const CKnownEntity* CHL1MPBotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(me, threat1, threat2);
}
