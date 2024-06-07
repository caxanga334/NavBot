#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include <mods/tf2/tf2lib.h>
#include "bot/tf2/tf2bot.h"
#include "tf2bot_scenario.h"
#include "tf2bot_tactical.h"
#include "tf2bot_find_ammo_task.h"
#include "tf2bot_find_health_task.h"

#undef max
#undef min
#undef clamp // undef mathlib macros

static ConVar sm_navbot_tf_ai_low_health_percent("sm_navbot_tf_ai_low_health_percent", "0.5", FCVAR_GAMEDLL, "If the bot health is below this percentage, the bot should retreat for health", true, 0.0f, true, 1.0f);

AITask<CTF2Bot>* CTF2BotTacticalTask::InitialNextTask()
{
	return new CTF2BotScenarioTask;
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

