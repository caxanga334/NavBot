#include <extension.h>
#include <bot/blackmesa/bmbot.h>
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
	auto threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

	if (threat)
	{
		bot->GetInventoryInterface()->SelectBestWeaponForThreat(threat.get());
		bot->FireWeaponAtEnemy(threat.get(), true);
	}

	return Continue();
}

Vector CBlackMesaBotMainTask::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, CBaseExtPlayer* player, DesiredAimSpot desiredAim)
{
	auto weapon = static_cast<CBlackMesaBot*>(me)->GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon)
	{
		return vec3_origin;
	}

	if (player != nullptr)
	{
		Vector result;

		bool secondary = false;

		if (me->GetControlInterface()->GetLastUsedAttackType() == IPlayerInput::AttackType::ATTACK_SECONDARY)
		{
			secondary = true;
		}

		if (AimAtEnemyPlayer(player, static_cast<CBlackMesaBot*>(me), result, weapon.get(), desiredAim, secondary))
		{
			return result;
		}
	}

	// default to aiming at the entity's center
	return UtilHelpers::getWorldSpaceCenter(entity);
}

std::shared_ptr<const CKnownEntity> CBlackMesaBotMainTask::SelectTargetThreat(CBaseBot* baseBot, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2)
{
	CBlackMesaBot* me = static_cast<CBlackMesaBot*>(baseBot);

	// check for cases where one of them is NULL

	if (threat1 && !threat2)
	{
		return threat1;
	}
	else if (!threat1 && threat2)
	{
		return threat2;
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

bool CBlackMesaBotMainTask::AimAtEnemyPlayer(CBaseExtPlayer* them, CBlackMesaBot* me, Vector& out, CBotWeapon* weapon, DesiredAimSpot desiredAim, bool isSecondaryAttack)
{
	/* TO-DO: Ballistics */

	float range = me->GetRangeTo(them->GetEntity());
	Vector aimat;

	switch (desiredAim)
	{
	case IDecisionQuery::AIMSPOT_ABSORIGIN:
		aimat = them->GetAbsOrigin();
		break;
	case IDecisionQuery::AIMSPOT_HEAD:
	{
		auto hdr = UtilHelpers::GetEntityModelPtr(them->GetEdict());

		if (!hdr)
		{
			aimat = them->WorldSpaceCenter();
			break;
		}

		/* In Black Mesa, the head isn't at the eye origin due to animations */

		int bone = UtilHelpers::LookupBone(hdr.get(), "ValveBiped.Bip01_Head1");

		if (bone < 0)
		{
#ifdef EXT_DEBUG
			const char* modelname = them->GetEdict()->GetIServerEntity()->GetModelName().ToCStr();
			smutils->LogError(myself, "Model \"%s\" doesn't have head bone \"ValveBiped.Bip01_Head1\"!", modelname);
#endif // EXT_DEBUG

			aimat = them->GetEyeOrigin();
			break;
		}

		QAngle angles;
		UtilHelpers::GetBonePosition(them->GetEntity(), bone, aimat, angles);
		aimat = aimat + weapon->GetWeaponInfo()->GetHeadShotAimOffset();
		break;
	}
	default:
		aimat = them->WorldSpaceCenter();
		break;
	}


	out = aimat;
	return true;
}

#ifdef EXT_DEBUG

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotMainTask::OnTestEventPropagation(CBlackMesaBot* bot)
{
	edict_t* host = UtilHelpers::GetListenServerHost();

	return TryPauseFor(new CBlackMesaBotDebugMoveToOriginTask(host->GetCollideable()->GetCollisionOrigin()), PRIORITY_CRITICAL, "Debug command received!");
}

CBlackMesaBotDebugMoveToOriginTask::CBlackMesaBotDebugMoveToOriginTask(const Vector& destination)
{
	m_goal = destination;
	m_failures = 0;
}

TaskResult<CBlackMesaBot> CBlackMesaBotDebugMoveToOriginTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	CBlackMesaBotPathCost cost(bot);
	m_nav.Update(bot, m_goal, cost);

	return Continue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotDebugMoveToOriginTask::OnMoveToFailure(CBlackMesaBot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failures > 20)
	{
		return TryDone(PRIORITY_HIGH, "Too many path failures, aborting!");
	}

	return TryContinue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotDebugMoveToOriginTask::OnMoveToSuccess(CBlackMesaBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Goal reached!");
}

#endif