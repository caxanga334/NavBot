#include NAVBOT_PCH_FILE
#include <bot/css/cssbot.h>
#include <bot/bot_shared_convars.h>
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include "cssbot_tactical_task.h"
#include "cssbot_main_task.h"

AITask<CCSSBot>* CCSSBotMainTask::InitialNextTask(CCSSBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CCSSBotTacticalTask;
}

TaskResult<CCSSBot> CCSSBotMainTask::OnTaskUpdate(CCSSBot* bot)
{
	return Continue();
}

TaskEventResponseResult<CCSSBot> CCSSBotMainTask::OnDebugMoveToCommand(CCSSBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CCSSBot, CCSSBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

TaskEventResponseResult<CCSSBot> CCSSBotMainTask::OnKilled(CCSSBot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CBotSharedDeadTask<CCSSBot, CCSSBotMainTask>(), PRIORITY_CRITICAL, "I am dead!");
}

Vector CCSSBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aim.SelectAimPosition(static_cast<CCSSBot*>(me), entity, desiredAim);
}

const CKnownEntity* CCSSBotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(me, threat1, threat2);
}
