#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include <bot/tf2/tasks/sniper/tf2bot_task_sniper_move_to_sniper_spot.h>
#include <bot/tf2/tasks/spy/tf2bot_task_spy_infiltrate.h>
#include <bot/tf2/tasks/scenario/deathmatch/tf2bot_deathmatch.h>
#include "scenario/controlpoints/tf2bot_controlpoints_monitor.h"
#include "scenario/payload/tf2bot_task_push_payload.h"
#include "scenario/mvm/tf2bot_mvm_idle.h"

#undef max
#undef min
#undef clamp // undef mathlib macros

static ConVar sm_navbot_tf_ai_low_health_percent("sm_navbot_tf_ai_low_health_percent", "0.5", FCVAR_GAMEDLL, "If the bot health is below this percentage, the bot should retreat for health", true, 0.0f, true, 1.0f);

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask(CTF2Bot* bot)
{
	return CTF2BotTacticalTask::SelectScenarioTask(bot);
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_ammochecktimer.Start(librandom::generate_random_float(5.0f, 10.0f));
	m_healthchecktimer.Start(librandom::generate_random_float(1.0f, 4.0f));

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTacticalTask::OnTaskUpdate(CTF2Bot* bot)
{
	// low ammo and health check
	if (bot->GetBehaviorInterface()->ShouldRetreat(bot) != ANSWER_NO && bot->GetBehaviorInterface()->ShouldHurry(bot) != ANSWER_YES)
	{
		if (m_healthchecktimer.IsElapsed())
		{
			m_healthchecktimer.Start(librandom::generate_random_float(1.0f, 4.0f));

			if (bot->GetHealthPercentage() <= sm_navbot_tf_ai_low_health_percent.GetFloat())
			{
				return PauseFor(new CTF2BotFindHealthTask, "I'm low on health!");
			}
		}

		if (m_ammochecktimer.IsElapsed())
		{
			m_ammochecktimer.Start(librandom::generate_random_float(5.0f, 10.0f));

			if (bot->IsAmmoLow())
			{
				return PauseFor(new CTF2BotFindAmmoTask, "I'm low on ammo!");
			}
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

	if (me->GetHealthPercentage() <= sm_navbot_tf_ai_low_health_percent.GetFloat())
	{
		return ANSWER_YES;
	}

	if (me->IsAmmoLow())
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

AITask<CTF2Bot>* CTF2BotTacticalTask::SelectScenarioTask(CTF2Bot* me, bool skipClassBehavior)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMIdleTask; // In MvM, all bots start with this
	}

	if (!skipClassBehavior)
	{
		AITask<CTF2Bot>* task = CTF2BotTacticalTask::SelectClassTask(me);

		if (task != nullptr)
		{
			return task;
		}
	}

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_CTF:
		return new CTF2BotCTFMonitorTask;
	case TeamFortress2::GameModeType::GM_PL:
		return new CTF2BotPushPayloadTask;
	case TeamFortress2::GameModeType::GM_CP:
	case TeamFortress2::GameModeType::GM_ADCP:
	case TeamFortress2::GameModeType::GM_ARENA: // arena may need it's own task
	case TeamFortress2::GameModeType::GM_TC: // same for TC
		return new CTF2BotControlPointMonitorTask;
	case TeamFortress2::GameModeType::GM_MVM:
		return new CTF2BotMvMIdleTask;
	default:
		break;
	}

	return new CTF2BotDeathmatchScenarioTask;
}

AITask<CTF2Bot>* CTF2BotTacticalTask::SelectClassTask(CTF2Bot* me)
{
	auto myclass = me->GetMyClassType();

	switch (myclass)
	{
	case TeamFortress2::TFClass_Sniper:
		return new CTF2BotSniperMoveToSnipingSpotTask;
	case TeamFortress2::TFClass_Medic:
		return new CTF2BotMedicMainTask;
	case TeamFortress2::TFClass_Spy:
		break; // TODO
	case TeamFortress2::TFClass_Engineer:
		return new CTF2BotEngineerMainTask;
	default:
		break;
	}

	return nullptr;
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* inflictor = info.GetInflictor();

		if (inflictor != nullptr)
		{
			int index = gamehelpers->EntityToBCompatRef(inflictor);

			if (index > 0 && index <= MAX_EDICTS)
			{
				Vector pos = UtilHelpers::getWorldSpaceCenter(inflictor);
				bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 2.0f, "Aiming at damage source!");
			}
		}
	}

	return TryContinue(PRIORITY_IGNORED);
}

TaskEventResponseResult<CTF2Bot> CTF2BotTacticalTask::OnVoiceCommand(CTF2Bot* bot, CBaseEntity* subject, int command)
{
	int skill = bot->GetDifficultyProfile()->GetGameAwareness();

	if (skill <= 5)
	{
		return TryContinue(PRIORITY_IGNORED); // this one doesn't know about VCs yet
	}

	TeamFortress2::TFTeam theirteam = static_cast<TeamFortress2::TFTeam>(entityprops::GetEntityTeamNum(subject));

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
	} // TO-DO: Allow bots to respond to some voice commands (IE: help)

	return TryContinue(PRIORITY_IGNORED);
}

