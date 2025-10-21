#include NAVBOT_PCH_FILE
#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/prediction.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/bot_shared_utils.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_debug_move_to_origin.h>
#include "tf2bot_dead.h"
#include "tf2bot_taunting.h"
#include "tf2bot_setuptime.h"
#include "tf2bot_tactical.h"
#include "tf2bot_maintask.h"

#undef max
#undef min
#undef clamp

AITask<CTF2Bot>* CTF2BotMainTask::InitialNextTask(CTF2Bot* bot)
{
	if (cvar_bot_disable_behavior.GetBool())
	{
		return nullptr;
	}

	return new CTF2BotTacticalTask;
}

TaskResult<CTF2Bot> CTF2BotMainTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (CTeamFortress2Mod::GetTF2Mod()->IsInSetup())
	{
		if (CTeamFortress2Mod::GetTF2Mod()->GetTeamRole(bot->GetMyTFTeam()) == TeamFortress2::TeamRoles::TEAM_ROLE_ATTACKERS &&
			bot->GetMyClassType() != TeamFortress2::TFClass_Medic)
		{
			return SwitchTo(new CTF2BotSetupTimeTask, "In setup time. Starting setup time behavior!");
		}
	}

	if (tf2lib::IsPlayerInCondition(bot->GetIndex(), TeamFortress2::TFCond::TFCond_Taunting))
	{
		return PauseFor(new CTF2BotTauntingTask, "Taunting!");
	}

	if (entprops->GameRules_GetRoundState() == RoundState_Preround)
	{
		bot->GetMovementInterface()->ClearStuckStatus("PREROUND"); // players are frozen during pre-round, don't get stuck
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMainTask::OnDebugMoveToCommand(CTF2Bot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CTF2Bot, CTF2BotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

const CKnownEntity* CTF2BotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	// Handle cases where one of them is NULL
	if (threat1 && !threat2)
	{
		return threat1;
	}
	if (!threat1 && threat2)
	{
		return threat2;
	}
	if (threat1 == threat2)
	{
		return threat1; // if both are the same, return threat1
	}

	return InternalSelectTargetThreat(static_cast<CTF2Bot*>(me), threat1, threat2);
}

Vector CTF2BotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return m_aimhelper.SelectAimPosition(static_cast<CTF2Bot*>(me), entity, desiredAim);
}

TaskEventResponseResult<CTF2Bot> CTF2BotMainTask::OnKilled(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	int teamID = static_cast<int>(bot->GetMyTFTeam());
	bool isSniper = false;
	CNavArea* area = bot->GetLastKnownNavArea();

	CBaseEntity* attacker = info.GetAttacker();

	if (attacker && UtilHelpers::IsPlayer(attacker) && tf2lib::GetPlayerClassType(UtilHelpers::IndexOfEntity(attacker)) == TeamFortress2::TFClassType::TFClass_Sniper)
	{
		isSniper = true;
	}

	float danger = isSniper ? CNavArea::ADD_DANGER_SNIPER : CNavArea::ADD_DANGER_KILLED;

	botsharedutils::SpreadDangerToNearbyAreas spread(area, teamID, 600.0f, danger, CNavArea::MAX_DANGER_ONKILLED);
	spread.Execute();

	return TrySwitchTo(new CTF2BotDeadTask, PRIORITY_MANDATORY, "I am dead!");
}

const CKnownEntity* CTF2BotMainTask::InternalSelectTargetThreat(CTF2Bot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	const CBotWeapon* weapon = me->GetInventoryInterface()->GetActiveBotWeapon();
	bool isMelee = (weapon != nullptr && weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).IsMelee());

	if (isMelee)
	{
		return botsharedutils::threat::SelectNearestThreat(me, threat1, threat2);
	}

	if (IsImmediateThreat(me, threat1))
	{
		return threat1;
	}

	if (IsImmediateThreat(me, threat2))
	{
		return threat2;
	}

	return botsharedutils::threat::SelectNearestThreat(me, threat1, threat2);
}

bool CTF2BotMainTask::IsImmediateThreat(CTF2Bot* me, const CKnownEntity* threat) const
{
	if (!threat->IsVisibleNow())
	{
		return false;
	}

	Vector to = (me->GetAbsOrigin() - threat->GetLastKnownPosition());
	float range = to.NormalizeInPlace();

	constexpr float MIN_RANGE_TO_CLASS_BASED_TARGETS = 900.0f;
	constexpr float SPY_DANGER_RANGE = 300.0f;

	if (threat->IsPlayer())
	{
		const CBaseExtPlayer* player = threat->GetPlayerInstance();
		TeamFortress2::TFClassType theirclass = tf2lib::GetPlayerClassType(player->GetIndex());

		if (theirclass == TeamFortress2::TFClassType::TFClass_Sniper)
		{
			Vector sniperForward;
			player->EyeVectors(&sniperForward);

			if (DotProduct(to, sniperForward) > 0.0f)
			{
				return true; // sniper is looking in my direction!
			}
		}

		// Sniper can see a medic's head
		if (me->GetDifficultyProfile()->GetGameAwareness() >= 75 && theirclass == TeamFortress2::TFClassType::TFClass_Medic &&
			me->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Sniper &&
			me->IsLineOfFireClear(player->GetEyeOrigin()))
		{
			return true;
		}

		if (me->GetDifficultyProfile()->GetGameAwareness() >= 60 && range <= MIN_RANGE_TO_CLASS_BASED_TARGETS)
		{
			if (theirclass == TeamFortress2::TFClassType::TFClass_Medic)
			{
				return true;
			}

			if (theirclass == TeamFortress2::TFClassType::TFClass_Engineer)
			{
				return true;
			}

			if (theirclass == TeamFortress2::TFClassType::TFClass_Spy && range <= SPY_DANGER_RANGE)
			{
				return true;
			}
		}
	}

	constexpr float SENTRY_RANGE = 1100.0f;

	if (range <= SENTRY_RANGE && threat->IsEntityOfClassname("obj_sentrygun"))
	{
		return true;
	}

	return botsharedutils::threat::IsImmediateThreat(me, threat);
}
