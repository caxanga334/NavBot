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

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		bot->GetInventoryInterface()->SelectBestWeaponForThreat(threat);
		bot->FireWeaponAtEnemy(threat, true);
		bot->DodgeEnemies(threat);
	}
	else
	{
		bot->HandleWeaponsNoThreat();
	}

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotMainTask::OnDebugMoveToCommand(CDoDSBot* bot, const Vector& moveTo)
{
	return TryPauseFor(new CBotSharedDebugMoveToOriginTask<CDoDSBot, CDoDSBotPathCost>(bot, moveTo), PRIORITY_CRITICAL, "Responding to debug command!");
}

Vector CDoDSBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	auto weapon = me->GetInventoryInterface()->GetActiveBotWeapon();
	constexpr std::string_view headbone{ "ValveBiped.Bip01_Head1" };

	if (weapon)
	{
		WeaponInfo::AttackFunctionType type = WeaponInfo::AttackFunctionType::PRIMARY_ATTACK;

		if (me->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
		{
			type = WeaponInfo::AttackFunctionType::SECONDARY_ATTACK;
		}

		if (UtilHelpers::IsPlayer(entity))
		{
			if (weapon->GetWeaponInfo()->GetAttackInfo(type).IsHitscan())
			{
				return botsharedutils::aiming::AimAtPlayerWithHitScan(me, entity, desiredAim, weapon, headbone.data());
			}
			else if (weapon->GetWeaponInfo()->GetAttackInfo(type).IsBallistic()) // ballistics needs to be checked first, every ballistic weapon is a projectile weapon
			{
				return botsharedutils::aiming::AimAtPlayerWithBallistic(me, entity, desiredAim, weapon, headbone.data());
			}
			else if (weapon->GetWeaponInfo()->GetAttackInfo(type).IsProjectile())
			{
				return botsharedutils::aiming::AimAtPlayerWithProjectile(me, entity, desiredAim, weapon, headbone.data());
			}
		}
	}

	return UtilHelpers::getWorldSpaceCenter(entity);
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
