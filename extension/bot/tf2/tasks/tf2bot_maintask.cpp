#include <algorithm>

#include <extension.h>
#include <manager.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/prediction.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_traces.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
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

	auto sensor = bot->GetSensorInterface();
	const CKnownEntity* threat = sensor->GetPrimaryKnownThreat();

	if (threat) // I have an enemy
	{
		bot->GetInventoryInterface()->SelectBestWeaponForThreat(threat);
		bot->FireWeaponAtEnemy(threat, true);
	}

	if (entprops->GameRules_GetRoundState() == RoundState_Preround)
	{
		bot->GetMovementInterface()->ClearStuckStatus("PREROUND"); // players are frozen during pre-round, don't get stuck
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMainTask::OnDebugMoveToHostCommand(CTF2Bot* bot)
{
#ifdef EXT_DEBUG
	auto host = gamehelpers->EdictOfIndex(1);
	CBaseExtPlayer player(host);
	Vector goal = player.GetAbsOrigin();

	return TryPauseFor(new CTF2BotDevTask(goal), PRIORITY_CRITICAL, "Move to Origin debug task!");
#else
	return TryContinue();
#endif // EXT_DEBUG
}

const CKnownEntity* CTF2BotMainTask::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	// Handle cases where one of them is NULL
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

	return InternalSelectTargetThreat(static_cast<CTF2Bot*>(me), threat1, threat2);
}

Vector CTF2BotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	Vector aimat(0.0f, 0.0f, 0.0f);
	CTF2Bot* tf2bot = static_cast<CTF2Bot*>(me);
	auto tfweapon = tf2bot->GetInventoryInterface()->GetActiveTFWeapon();

	if (!tfweapon)
	{
		tf2bot->GetInventoryInterface()->RequestRefresh();
		return UtilHelpers::getWorldSpaceCenter(entity);
	}

	const WeaponAttackFunctionInfo* attackFunc = nullptr;

	if (tf2bot->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
	{
		attackFunc = &(tfweapon->GetTF2Info()->GetAttackInfo(WeaponInfo::SECONDARY_ATTACK));
	}
	else
	{
		attackFunc = &(tfweapon->GetTF2Info()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK));
	}

	if (UtilHelpers::IsPlayer(entity))
	{
		std::unique_ptr<CBaseExtPlayer> player = std::make_unique<CBaseExtPlayer>(UtilHelpers::BaseEntityToEdict(entity));

		if (attackFunc->IsBallistic())
		{
			AimAtPlayerWithBallisticWeapon(tf2bot, player.get(), aimat, desiredAim, tfweapon.get(), attackFunc);
		}
		else if (attackFunc->IsProjectile())
		{
			AimAtPlayerWithProjectileWeapon(tf2bot, player.get(), aimat, desiredAim, tfweapon.get(), attackFunc);
		}
		else
		{
			AimAtPlayerWithHitScanWeapon(tf2bot, player.get(), aimat, desiredAim, tfweapon.get(), attackFunc);
		}
	}
	else
	{
		aimat = UtilHelpers::getWorldSpaceCenter(entity);
	}

	return aimat;
}

TaskEventResponseResult<CTF2Bot> CTF2BotMainTask::OnKilled(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	return TrySwitchTo(new CTF2BotDeadTask, PRIORITY_MANDATORY, "I am dead!");
}

void CTF2BotMainTask::AimAtPlayerWithHitScanWeapon(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, DesiredAimSpot desiredAim, const CTF2BotWeapon* weapon, const WeaponAttackFunctionInfo* attackInfo)
{
	if (desiredAim == IDecisionQuery::AIMSPOT_HEAD)
	{
		player->GetHeadShotPosition("bip_head", result);
		result = result + weapon->GetTF2Info()->GetHeadShotAimOffset();
		return;
	}

	result = player->WorldSpaceCenter();
}

void CTF2BotMainTask::AimAtPlayerWithProjectileWeapon(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, DesiredAimSpot desiredAim, const CTF2BotWeapon* weapon, const WeaponAttackFunctionInfo* attackInfo)
{
	if (player->GetGroundEntity() == nullptr) // target is airborne
	{
		trace::CTraceFilterNoNPCsOrPlayers filter(me->GetEntity(), COLLISION_GROUP_NONE);
		trace_t tr;
		trace::line(player->GetAbsOrigin(), player->GetAbsOrigin() + Vector(0.0f, 0.0f, -256.0f), MASK_PLAYERSOLID, &filter, tr);

		if (tr.DidHit())
		{
			result = tr.endpos;
			return;
		}
	}

	// lead target

	float rangeBetween = me->GetRangeTo(player->GetAbsOrigin());

	constexpr float veryCloseRange = 150.0f;
	if (rangeBetween > veryCloseRange)
	{
		Vector targetPos = pred::SimpleProjectileLead(player->GetAbsOrigin(), player->GetAbsVelocity(), weapon->GetProjectileSpeed(), rangeBetween);

		if (me->GetSensorInterface()->IsLineOfSightClear(targetPos))
		{
			result = targetPos;
			return;
		}

		// try their head and hope
		result = pred::SimpleProjectileLead(player->GetEyeOrigin(), player->GetAbsVelocity(), weapon->GetProjectileSpeed(), rangeBetween);
		return;
	}

	result = player->GetEyeOrigin();
	return;
}

void CTF2BotMainTask::AimAtPlayerWithBallisticWeapon(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, DesiredAimSpot desiredAim, const CTF2BotWeapon* weapon, const WeaponAttackFunctionInfo* attackInfo)
{
	Vector enemyPos;

	if (desiredAim == IDecisionQuery::AIMSPOT_HEAD)
	{
		player->GetHeadShotPosition("bip_head", enemyPos);
		enemyPos = enemyPos + weapon->GetTF2Info()->GetHeadShotAimOffset();
	}
	else
	{
		// aim at the enemy's center
		enemyPos = player->WorldSpaceCenter();
	}

	Vector myEyePos = me->GetEyeOrigin();
	Vector enemyVel = player->GetAbsVelocity();
	float rangeTo = (myEyePos - enemyPos).Length();
	const float projSpeed = weapon->GetProjectileSpeed();
	const float gravity = weapon->GetProjectileGravity();

	Vector aimPos = pred::SimpleProjectileLead(enemyPos, enemyVel, projSpeed, rangeTo);
	rangeTo = (myEyePos - aimPos).Length();

	const float elevation_rate = RemapValClamped(rangeTo, attackInfo->GetBallisticElevationStartRange(), attackInfo->GetBallisticElevationEndRange(), attackInfo->GetBallisticElevationMinRate(), attackInfo->GetBallisticElevationMaxRate());
	float z = pred::GravityComp(rangeTo, gravity, elevation_rate);
	aimPos.z += z;

	result = aimPos;
}

const CKnownEntity* CTF2BotMainTask::InternalSelectTargetThreat(CTF2Bot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	// TO-DO: Add threat selection

	auto range1 = me->GetRangeTo(threat1->GetEdict());
	auto range2 = me->GetRangeTo(threat2->GetEdict());

	// temporary distance based for testing
	if (range1 < range2)
	{
		return threat1;
	}

	return threat2;
}

#ifdef EXT_DEBUG

CTF2BotDevTask::CTF2BotDevTask(const Vector& moveTo)
{
	m_goal = moveTo;
}

TaskResult<CTF2Bot> CTF2BotDevTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_repathtimer.Start(0.5f);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to compute path!");
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotDevTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Reset();

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return Done("Failed to compute path!");
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDevTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_repathtimer.Reset();

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to compute path!");
	}

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotDevTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal reached!");
}

#endif // EXT_DEBUG