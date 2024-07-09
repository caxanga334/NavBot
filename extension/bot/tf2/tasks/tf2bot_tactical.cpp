#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"
#include <bot/tf2/tasks/scenario/tf2bot_map_ctf.h>
#include <bot/tf2/tasks/medic/tf2bot_medic_main_task.h>
#include <bot/tf2/tasks/engineer/tf2bot_engineer_main.h>
#include <bot/tf2/tasks/sniper/tf2bot_task_sniper_move_to_sniper_spot.h>
#include "scenario/payload/tf2bot_task_push_payload.h"
#include "scenario/mvm/tf2bot_mvm_idle.h"

#undef max
#undef min
#undef clamp // undef mathlib macros

static ConVar sm_navbot_tf_ai_low_health_percent("sm_navbot_tf_ai_low_health_percent", "0.5", FCVAR_GAMEDLL, "If the bot health is below this percentage, the bot should retreat for health", true, 0.0f, true, 1.0f);

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask(CTF2Bot* bot)
{
	return SelectScenarioTask(bot);
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

AITask<CTF2Bot>* CTF2BotTacticalTask::SelectScenarioTask(CTF2Bot* me)
{
	auto tf2mod = CTeamFortress2Mod::GetTF2Mod();
	auto gm = tf2mod->GetCurrentGameMode();
	auto myclass = me->GetMyClassType();

	if (gm == TeamFortress2::GameModeType::GM_MVM)
	{
		return new CTF2BotMvMIdleTask; // In MvM, all bots start with this
	}

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

	switch (gm)
	{
	case TeamFortress2::GameModeType::GM_CTF:
		return new CTF2BotCTFMonitorTask;
	case TeamFortress2::GameModeType::GM_PL:
		return new CTF2BotPushPayloadTask;
	default:
		break;
	}

	return nullptr;
}

