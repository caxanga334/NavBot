#include NAVBOT_PCH_FILE

#include <extension.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_convars.h>
#include <bot/interfaces/behavior_utils.h>
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"
#include "tf2bot_use_teleporter.h"
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/tasks_shared/bot_shared_retreat_from_threat.h>
#include <bot/tasks_shared/bot_shared_escort_entity.h>
#include "tf2bot_scenario_task.h"

#undef max
#undef min
#undef clamp // undef mathlib macros

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask(CTF2Bot* bot)
{
	return new CTF2BotScenarioTask;
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(5.0f);
	m_teleportertimer.Start(0.2f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskUpdate(CTF2Bot* bot)
{
	const bool shouldRetreat = bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO;

	// low ammo and health check
	if (shouldRetreat && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthchecktimer.IsElapsed())
		{
			m_healthchecktimer.StartRandom(1.0f, 5.0f);

			if (bot->GetHealthState() != CBaseBot::HealthState::HEALTH_OK)
			{
				CBaseEntity* source = nullptr;

				if (CTF2BotFindHealthTask::IsPossible(bot, &source))
				{
					return PauseFor(new CTF2BotFindHealthTask(source), "I'm low on health!");
				}
			}
		}

		if (!sm_navbot_bot_no_ammo_check.GetBool() && m_ammochecktimer.IsElapsed())
		{
			m_ammochecktimer.StartRandom(1.0f, 5.0f);

			if (bot->GetInventoryInterface()->IsAmmoLow(false))
			{
				CBaseEntity* source = nullptr;

				if (CTF2BotFindAmmoTask::IsPossible(bot, &source))
				{
					return PauseFor(new CTF2BotFindAmmoTask(source), "I'm low on ammo!");
				}
			}
		}
	}

	if (m_teleportertimer.IsElapsed())
	{
		if (bot->GetTimeSinceLastSpawn() <= 5.0f)
		{
			m_teleportertimer.Start(0.2f); // frequent checks after a respawn
		}
		else
		{
			m_teleportertimer.StartRandom(2.0f, 10.0f);
		}

		CBaseEntity* teleporter = nullptr;

		if (CTF2BotUseTeleporterTask::IsPossible(bot, &teleporter))
		{
			return PauseFor(new CTF2BotUseTeleporterTask(teleporter), "Using teleporter!");
		}
	}

	if (shouldRetreat && m_retreatCooldown.IsElapsed())
	{
		const CTF2BotSensor* sensor = bot->GetSensorInterface();
		const int enemies = sensor->GetVisibleEnemiesCount();
		const int allies = sensor->GetKnownAllyCount();

		if (enemies > 1 && (enemies - allies) >= bot->GetDifficultyProfile()->GetRetreatFromNumericalDisadvantageThreshold())
		{
			m_retreatCooldown.Start(120.0f); // cooldown to avoid constant retreats
			return PauseFor(new CBotSharedRetreatFromThreatTask<CTF2Bot, CTF2BotPathCost>(bot, true), "Retreating due to numerical disadvantage!");
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskResume(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(5.0f);
	m_healthchecktimer.Start(0.5f);

	return Continue();
}

QueryAnswerType CTF2BotTacticalTask::ShouldRetreat(CBaseBot* base)
{
	CTF2Bot* me = static_cast<CTF2Bot*>(base);

	if (tf2lib::IsPlayerInvulnerable(me->GetIndex()))
	{
		return ANSWER_NO;
	}

	if (me->GetHealthState() != CBaseBot::HealthState::HEALTH_OK)
	{
		return ANSWER_YES;
	}

	if (me->GetInventoryInterface()->IsAmmoLow(false))
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnNavAreaChanged(CTF2Bot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CTF2Bot, CTF2BotPathCost);

	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* attacker = info.GetAttacker();

		if (attacker && !bot->GetSensorInterface()->IsIgnored(attacker))
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(attacker);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 2.0f, "Aiming at damage source!");
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	int skill = bot->GetDifficultyProfile()->GetGameAwareness();

	if (skill <= 5)
	{
		return TryContinue(PRIORITY_DONT_CARE); // this one doesn't know about VCs yet
	}

	TeamFortress2::TFTeam theirteam = tf2lib::GetEntityTFTeam(subject);
	TeamFortress2::VoiceCommandsID vcmd = static_cast<TeamFortress2::VoiceCommandsID>(command);

	if (skill >= 75 && theirteam != bot->GetMyTFTeam())
	{
		if (bot->GetRangeTo(subject) < bot->GetSensorInterface()->GetMaxHearingRange())
		{
			CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(subject);
			Vector pos = UtilHelpers::getWorldSpaceCenter(subject);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_ALERT, 2.0f, "Looking at hostile voice command source position!");

			if (known)
			{
				// this will update the entity position and mark it as heard
				known->UpdateHeard();
			}
		}
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnObjectSapped(CTF2Bot* bot, CBaseEntity* owner, CBaseEntity* saboteur)
{
	if (saboteur && bot->GetDifficultyProfile()->GetGameAwareness() > 10 && tf2lib::GetEntityTFTeam(saboteur) != bot->GetMyTFTeam() && bot->GetRangeTo(saboteur) <= bot->GetSensorInterface()->GetMaxHearingRange())
	{
		CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(saboteur);
		known->UpdateHeard();

		bot->GetSpyMonitorInterface()->DetectSpy(saboteur);
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

