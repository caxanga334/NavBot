#include NAVBOT_PCH_FILE
#include <bot/bot_shared_utils.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include <bot/tasks_shared/bot_shared_dead.h>
#include <bot/zps/zpsbot.h>
#include <mods/zps/zps_mod.h>
#include "zpsbot_tactical_task.h"
#include "zpsbot_main_task.h"

AITask<CZPSBot>* CZPSBotMainTask::InitialNextTask(CZPSBot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CZPSBotTacticalTask;
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
	return m_aimhelper.SelectAimPosition(static_cast<CZPSBot*>(me), entity, desiredAim);
}

QueryAnswerType CZPSBotMainTask::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
	const char* classname = gamehelpers->GetEntityClassname(item);

	if (UtilHelpers::StringMatchesPattern(classname, "weapon_*", 0))
	{
		Vector mins{ -12.0f, -12.0f, 1.0f };
		Vector maxs{ 12.0f, 12.0f, 16.0f };
		Vector pos = UtilHelpers::getEntityOrigin(item);

		trace_t tr;
		trace::CTraceFilterSimple filter{ me->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT };
		trace::hull(pos, pos, mins, maxs, MASK_PLAYERSOLID, &filter, tr);

		// some maps places items in hard to reach places, filter them out
		if (tr.fraction < 1.0f)
		{
			return ANSWER_NO;
		}

		CZPSBot* bot = static_cast<CZPSBot*>(me);

		std::string name{ classname };
		const WeaponInfo* info = extmanager->GetMod()->GetWeaponInfoManager()->GetClassnameInfo(name);

		const CBotWeapon* weapon = bot->GetInventoryInterface()->FindWeaponByClassname(classname, true);

		// bot already owns this weapon
		if (weapon != nullptr && weapon->HasAmmo(bot))
		{
			return ANSWER_NO;
		}

		weapon = bot->GetInventoryInterface()->FindWeaponBySlot(info->GetSlot());

		if (weapon != nullptr && weapon->HasAmmo(bot) /* && weapon->GetWeaponInfo()->GetPriority() >= info->GetPriority() */)
		{
			return ANSWER_NO;
		}
	}

	return ANSWER_YES;
}

TaskEventResponseResult<CZPSBot> CZPSBotMainTask::OnDebugMoveToCommand(CZPSBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CZPSBot, CZPSBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Debug command!");
}

TaskEventResponseResult<CZPSBot> CZPSBotMainTask::OnKilled(CZPSBot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CBotSharedDeadTask<CZPSBot, CZPSBotMainTask>(), PRIORITY_MANDATORY, "I am dead!");
}
