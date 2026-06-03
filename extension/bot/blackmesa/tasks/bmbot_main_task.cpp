#include NAVBOT_PCH_FILE
#include <bot/blackmesa/bmbot.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include <bot/bot_shared_utils.h>
#include "bmbot_tactical_task.h"
#include "bmbot_main_task.h"

CBlackMesaBotMainTask::CBlackMesaBotMainTask()
{
}

AITask<CBlackMesaBot>* CBlackMesaBotMainTask::InitialNextTask(CBlackMesaBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CBlackMesaBotTacticalTask;
}

TaskResult<CBlackMesaBot> CBlackMesaBotMainTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	return Continue();
}

Vector CBlackMesaBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aimhelper.SelectAimPosition(static_cast<CBlackMesaBot*>(me), entity, desiredAim);
}

const CKnownEntity* CBlackMesaBotMainTask::SelectTargetThreat(CBaseBot* baseBot, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return botsharedutils::threat::DefaultThreatSelection(baseBot, threat1, threat2);
}

QueryAnswerType CBlackMesaBotMainTask::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
	const char* classname = gamehelpers->GetEntityClassname(item);

	// is this a weapon?
	if (UtilHelpers::StringMatchesPattern(classname, "item_weapon_*", 0))
	{
		if (static_cast<CBlackMesaBot*>(me)->GetInventoryInterface()->OwnsThisWeapon(classname))
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotMainTask::OnDebugMoveToCommand(CBlackMesaBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CBlackMesaBot, CBlackMesaBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Debug command received!");
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotMainTask::OnKilled(CBlackMesaBot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CBotSharedDeadTask<CBlackMesaBot, CBlackMesaBotMainTask>, PRIORITY_MANDATORY, "I am dead!");
}

