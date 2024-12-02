#include <extension.h>
#include <util/helpers.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_taunting.h"

TaskResult<CTF2Bot> CTF2BotTauntingTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_endTauntTimer.StartRandom(6.0f, 12.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotTauntingTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_endTauntTimer.IsElapsed())
	{
		bot->DelayedFakeClientCommand("stop_taunt");
	}

	if (!tf2lib::IsPlayerInCondition(bot->GetIndex(), TeamFortress2::TFCond::TFCond_Taunting))
	{
		return Done("No longer taunting!");
	}

	return Continue();
}
