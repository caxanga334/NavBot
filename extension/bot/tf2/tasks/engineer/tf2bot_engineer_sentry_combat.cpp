#include NAVBOT_PCH_FILE
#include <bot/tf2/tf2bot.h>
#include <entities/tf2/tf_entities.h>
#include <bot/tasks_shared/bot_shared_attack_enemy.h>
#include "tf2bot_engineer_speedup_object.h"
#include "tf2bot_engineer_repair_object.h"
#include "tf2bot_engineer_sentry_combat.h"

TaskResult<CTF2Bot> CTF2BotEngineerSentryCombatTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

	if (!threat)
	{
		return Done("No threat!");
	}

	CBaseEntity* sentry = bot->GetMySentryGun();

	if (!sentry)
	{
		return SwitchTo(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Sentry destroyed, attacking enemy!");
	}

	tfentities::HObjectSentryGun object(sentry);

	if (object.GetPercentageConstructed() < 0.98f)
	{
		return PauseFor(new CTF2BotEngineerSpeedUpObjectTask(sentry), "Speeding up sentry construction!");
	}

	if (bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(threat->GetEntity())) > TOO_CLOSE_FOR_WRANGLER)
	{
		const CTF2BotWeapon* wrangler = bot->GetInventoryInterface()->GetTheWrangler();

		if (wrangler)
		{
			m_hasWrangler = true;
			bot->GetInventoryInterface()->EquipWeapon(wrangler);
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotEngineerSentryCombatTask::OnTaskUpdate(CTF2Bot* bot)
{
	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);
	CBaseEntity* sentry = bot->GetMySentryGun();

	if (!threat)
	{
		return Done("No threat!");
	}

	if (!sentry)
	{
		return SwitchTo(new CBotSharedAttackEnemyTask<CTF2Bot, CTF2BotPathCost>(bot), "Sentry destroyed, attacking enemy!");
	}

	tfentities::HObjectSentryGun object(sentry);
	auto control = bot->GetControlInterface();

	// not using the wrangler
	if (!m_hasWrangler)
	{
		if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) > 0)
		{
			if (object.GetHealthPercentage() < 0.9f)
			{
				return PauseFor(new CTF2BotEngineerRepairObjectTask(sentry), "Repairing my entry while in combat!");
			}
		}
	}
	else
	{
		const CTF2BotWeapon* wrangler = bot->GetInventoryInterface()->GetTheWrangler();

		if (!wrangler)
		{
			m_hasWrangler = false;
			return Continue();
		}

		if (bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) > 0)
		{
			if (object.GetAmmoShells() <= 0)
			{
				return PauseFor(new CTF2BotEngineerRepairObjectTask(sentry), "Refilling the sentry gun's ammo.");
			}
		}

		bot->GetInventoryInterface()->EquipWeapon(wrangler);
		control->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
		control->AimAt(threat->GetEntity(), IPlayerController::LookPriority::LOOK_COMBAT, 1.0f, "Aiming wrangler at enemy!");

		if (control->IsAimOnTarget() && CTF2BotEngineerSentryCombatTask::IsSentryLineOfFireClear(sentry, threat->GetEntity()))
		{
			control->PressAttackButton();
			control->PressSecondaryAttackButton();
		}
	}

	const Vector& vSentry = UtilHelpers::getEntityOrigin(sentry);
	Vector dir = UtilHelpers::math::BuildDirectionVector(vSentry, UtilHelpers::getWorldSpaceCenter(threat->GetEntity()));
	m_goal = vSentry - (dir * CTF2BotEngineerSentryCombatTask::BEHIND_SENTRY_DIST);
	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, m_goal, cost);

	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		NDebugOverlay::VertArrow(m_goal + Vector(0.0f, 0.0f, 72.0f), m_goal, 16.0f, 255, 255, 0, 255, true, 0.1f);
	}

	return Continue();
}

void CTF2BotEngineerSentryCombatTask::OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask)
{
	CBaseEntity* wrench = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee));

	if (wrench)
	{
		bot->SelectWeapon(wrench);
	}
}

QueryAnswerType CTF2BotEngineerSentryCombatTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (m_hasWrangler)
	{
		return ANSWER_NO;
	}

	return ANSWER_YES;
}

bool CTF2BotEngineerSentryCombatTask::IsSentryLineOfFireClear(CBaseEntity* sentry, CBaseEntity* threat)
{
	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	trace::line(UtilHelpers::getWorldSpaceCenter(sentry), UtilHelpers::getWorldSpaceCenter(threat), MASK_SHOT, &filter, tr);
	return !tr.startsolid && tr.fraction >= 1.0f;
}
